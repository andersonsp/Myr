#include "land.h"

// Much of this was taken from the paper "Improved Collision detection and Response" by Kasper Fauerby
// http://www.peroxide.dk/papers/collision/collision.pdf

static int get_lowest_root( float a, float b, float c, float max_r, float* root ) {
    // Check if a solution exists
    float determinant = b*b - 4.0f*a*c;
    if( determinant < 0.0f ) return 0;

    float sqrtD = sqrt( determinant );
    float r1 = ( -b - sqrtD ) / ( 2*a );
    float r2 = ( -b + sqrtD ) / ( 2*a );

    if( r1 > r2 ) { // Sort so x1 <= x2
        float temp = r2;
        r2 = r1;
        r1 = temp;
    }

    if( r1>0 && r1<max_r ) { // Get lowest root:
        *root = r1;
        return 1;
    }
    // It is possible that we want x2 - this can happen if x1 < 0
    if( r2 > 0 && r2 < max_r ) {
        *root = r2;
        return 1;
    }

    return 0; // No (valid) solutions
}

static float test_vertex( TraceInfo* trace, Vec* p, Vec* start, Vec* vel, float vel_sqr_len, float t ) {
    Vec v;
    vec_sub( &v, start, p );
    float b = 2.0* vec_dot( vel, &v );
    vec_sub( &v, p, start );
    float c = vec_dot( &v, &v ) - 1.0;
    float new_t;
    if( get_lowest_root(vel_sqr_len, b, c, t, &new_t) ) {
        trace->collision = 1;
        if( new_t < trace->t ) {
            trace->t = new_t;
            vec_scale( &trace->intersect_point, p, trace->radius );
        }
        return new_t;
    }
    return t;
}

static float test_edge( TraceInfo* trace, Vec* pa, Vec* pb, Vec* start, Vec* vel, float vel_sqr_len, float t ) {
    Vec edge, v;
    vec_sub( &edge, pb, pa  );
    vec_sub( &v, pa, start );

    float edge_sqr_len = vec_dot( &edge, &edge );
    float edge_dot_vel = vec_dot( &edge, vel );
    float edge_dot_sphere_vert = vec_dot( &edge, &v );

    float a = edge_sqr_len * -vel_sqr_len + edge_dot_vel * edge_dot_vel;
    float b = edge_sqr_len * (2.0 * vec_dot(vel, &v)) - 2.0*edge_dot_vel*edge_dot_sphere_vert;
    float c = edge_sqr_len * (1.0 - vec_dot(&v, &v)) + edge_dot_sphere_vert*edge_dot_sphere_vert;

    // Check for intersection against infinite line
    float new_t;
    if( get_lowest_root(a, b, c, t, &new_t) && new_t < trace->t ) {
        // Check if intersection against the line segment:
        float f = (edge_dot_vel*new_t - edge_dot_sphere_vert) / edge_sqr_len;
        if( f >= 0.0 && f <= 1.0 ) {
            trace->collision = 1;
            vec_scale( &v, &edge, f );
            vec_add( &v, &v, pa );
            trace->t = new_t;
            vec_scale( &trace->intersect_point, &v, trace->radius );
            return new_t;
        }
    }
    return t;
}

static int point_in_triangle(Vec* pt, Vec* p0, Vec* p1, Vec* p2 ) {
    Vec u, v, w;
    vec_sub( &u, p1, p0 );
    vec_sub( &v, p2, p0 );
    vec_sub( &w, pt, p0 );

    float uu = vec_dot( &u, &u );
    float uv = vec_dot( &u, &v );
    float vv = vec_dot( &v, &v );
    float wu = vec_dot( &w, &u );
    float wv = vec_dot( &w, &v );
    float d = uv * uv - uu * vv;

    float invD = 1 / d;
    float s = (uv * wv - vv * wu) * invD;
    if (s < 0 || s > 1) return 0;
    float t = (uv * wu - uu * wv) * invD;
    if (t < 0 || (s + t) > 1) return 0;

    return 1;
}

void trace_sphere_triangle( TraceInfo* trace, Vec* p0, Vec* p1, Vec* p2 ) {
    Vec vel = trace->scaled_vel;
    Vec start = trace->scaled_start;
    Vec ta, tb, tc, pab, pac, norm;

    // Scale the triangle points so that we're colliding against a unit-radius sphere.
    vec_scale( &ta, p0, trace->inv_radius );
    vec_scale( &tb, p1, trace->inv_radius );
    vec_scale( &tc, p2, trace->inv_radius );

    // Calculate triangle normal. This may be better to do as a pre-process
    vec_sub( &pab, &tb, &ta );
    vec_sub( &pac, &tc, &ta );
    vec_cross( &norm, &pab, &pac );
    vec_normalize( &norm, &norm );
    float plane_d = -(norm.x*ta.x+norm.y*ta.y+norm.z*ta.z);

    // Colliding against the backface of the triangle
    if( vec_dot(&norm, &trace->norm_vel) >= 0 ) return;

    // Get interval of plane intersection:
    float t0, t1;
    int embedded = 0;

    // Calculate the signed distance from sphere position to triangle plane
    float dist_to_plane = vec_dot( &start, &norm ) + plane_d;

    // cache this as we're going to use it a few times below:
    float norm_dot_vel = vec_dot( &norm, &vel );

    if( norm_dot_vel == 0.0 ) {
        // Sphere is travelling parrallel to the plane:
        if( abs(dist_to_plane ) >= 1.0 ) {
            // Sphere is not embedded in plane, No collision possible
            return;
        } else {
            // Sphere is completely embedded in plane. It intersects in the whole range [0..1]
            embedded = 1;
            t0 = 0.0;
            t1 = 1.0;
        }
    } else {
        // Calculate intersection interval:
        t0 = (-1.0-dist_to_plane)/norm_dot_vel;
        t1 = ( 1.0-dist_to_plane)/norm_dot_vel;
        // Swap so t0 < t1
        if( t0 > t1 ) {
            float temp = t1;
            t1 = t0;
            t0 = temp;
        }
        // Check that at least one result is within range:
        if (t0 > 1.0 || t1 < 0.0) return; // No collision possible

        // Clamp to [0,1]
        if( t0 < 0.0 ) t0 = 0.0;
        if( t1 < 0.0 ) t1 = 0.0;
        if( t0 > 1.0 ) t0 = 1.0;
        if( t1 > 1.0 ) t1 = 1.0;
    }

    // If the closest possible collision point is further away than an already detected collision
    // then there's no point in testing further.
    if( t0 >= trace->t ) return;

    // t0 and t1 now represent the range of the sphere movement during which it intersects with
    // the triangle plane. Collisions cannot happen outside that range.

    // Check for collision againt the triangle face:
    if( !embedded ) {
      Vec plane_intersect, v;
      // Calculate the intersection point with the plane
      vec_sub( &plane_intersect, &start, &norm);
      vec_scale( &v, &vel, t0 );
      vec_add( &plane_intersect, &plane_intersect, &v );

      // Is that point inside the triangle?
      if( point_in_triangle(&plane_intersect, &ta, &tb, &tc) ) {
          trace->collision = 1;
          if( t0 < trace->t ) {
              trace->t = t0;
              vec_scale( &trace->intersect_point, &plane_intersect, trace->radius );
          }
          // Collisions against the face will always be closer than vertex or edge collisions
          // so we can stop checking now.
          return;
      }
    }

    float vel_sqr_len = vec_dot( &vel, &vel );
    float t = trace->t;

    // Check for collision againt the triangle vertices:
    t = test_vertex( trace, &ta, &start, &vel, vel_sqr_len, t );
    t = test_vertex( trace, &tb, &start, &vel, vel_sqr_len, t );
    t = test_vertex( trace, &tc, &start, &vel, vel_sqr_len, t );

    // Check for collision against the triangle edges:
    t = test_edge( trace, &ta, &tb, &start, &vel, vel_sqr_len, t );
    t = test_edge( trace, &tb, &tc, &start, &vel, vel_sqr_len, t );
    test_edge( trace, &tc, &ta, &start, &vel, vel_sqr_len, t );
}


 // * Point on ray:
 // *     pos + t * dir, t in R
 // * Point in triangle:
 // *     (1 - u - v) * v0 + u * v1 + v * v2, u >= 0, v >= 0, u + v <= 1
void trace_ray_triangle( TraceInfo* ti, Vec* v0, Vec* v1, Vec* v2 ) {
    Vec eu, ev, cv, vt, cu;
    vec_sub( &eu, v1, v0 );
    vec_sub( &ev, v2, v0 );

    vec_cross( &cv, &ti->vel, &ev );
    float det = vec_dot( &eu, &cv );
    if( det <= 0 ) return; // backface cull

    vec_sub( &vt, &ti->start, v0 );
    float u = vec_dot( &vt, &cv );
    if( u < 0 || u > det ) return;

    vec_cross( &cu, &vt, &eu );
    float v = vec_dot( &ti->vel, &cu );
    if( v < 0 || u+v > det ) return;

    float t = vec_dot( &ev, &cu );
    if( t < 0 ) return;

    t /= det;
    if( t < ti->t ) ti->t = t;

    ti->collision = 1;
    vec_add( &ti->intersect_point, &ti->start, vec_scale(&ti->intersect_point, &ti->vel, t) );
}

void trace_init( TraceInfo* trace, Vec* start, Vec* vel, float radius ){
    trace->start = *start;
    trace->vel = trace->norm_vel = *vel;
    trace->collision = 0;
    trace->t = 1.0;
    if( radius > 0.0 ) {
        vec_normalize( &trace->norm_vel, &trace->norm_vel );
        trace->radius = radius;
        trace->inv_radius = 1/radius;

        vec_scale( &trace->scaled_start, &trace->start, trace->inv_radius );
        vec_scale( &trace->scaled_vel, &trace->vel, trace->inv_radius );
    }
}

void trace_end( TraceInfo* trace, Vec* end ){
    Vec tmp;
    vec_add( end, &trace->start, vec_scale(&tmp, &trace->vel, trace->t) );
}

float trace_dist( TraceInfo* trace ) {
    Vec tmp;
    return vec_len( vec_scale(&tmp, &trace->vel, trace->t) );
}
