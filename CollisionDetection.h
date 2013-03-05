#ifndef COLLISION_DETECTION_H
#define COLLISION_DETECTION_H

#include "Board.h"
#include "Biconvex.h"
#include "Intersection.h"
#include <stdint.h>

// --------------------------------------------------------------------------

struct StaticContact
{
    RigidBody * rigidBody;
    vec3f point;
    vec3f normal;
    float depth;
};

struct DynamicContact
{
    RigidBody * a;
    RigidBody * b;
    vec3f point;
    vec3f normal;
};

inline bool StoneBoardCollision( const Biconvex & biconvex,
                                 const Board & board, 
                                 RigidBody & rigidBody,
                                 StaticContact & contact );

inline bool StoneFloorCollision( const Biconvex & biconvex,
                                 const Board & board, 
                                 RigidBody & rigidBody,
                                 StaticContact & contact );

// --------------------------------------------------------------------------

inline void ClosestFeaturesBiconvexPlane_LocalSpace( vec3f planeNormal,
                                                     float planeDistance,
                                                     const Biconvex & biconvex,
                                                     vec3f & biconvexPoint,
                                                     vec3f & biconvexNormal,
                                                     vec3f & planePoint )
{
    const float sphereDot = biconvex.GetSphereDot();
    const float planeNormalDot = fabs( dot( vec3f(0,1,0), planeNormal ) );
    if ( planeNormalDot > sphereDot )
    {
        // sphere surface collision
        const float sphereRadius = biconvex.GetSphereRadius();
        const float sphereOffset = planeNormal.y() < 0 ? -biconvex.GetSphereOffset() : +biconvex.GetSphereOffset();
        vec3f sphereCenter( 0, sphereOffset, 0 );
        biconvexPoint = sphereCenter - normalize( planeNormal ) * sphereRadius;
        biconvexNormal = normalize( biconvexPoint - sphereCenter );
    }
    else
    {
        // circle edge collision
        const float circleRadius = biconvex.GetCircleRadius();
        biconvexPoint = normalize( vec3f( -planeNormal.x(), 0, -planeNormal.z() ) ) * circleRadius;
        biconvexNormal = normalize( biconvexPoint );
    }

    planePoint = biconvexPoint - planeNormal * ( dot( biconvexPoint, planeNormal ) - planeDistance );
}

inline void ClosestFeaturesStoneBoard( const Board & board, 
                                       const Biconvex & biconvex, 
                                       const RigidBodyTransform & biconvexTransform,
                                       vec3f & stonePoint,
                                       vec3f & stoneNormal,
                                       vec3f & boardPoint,
                                       vec3f & boardNormal )
{
    vec3f biconvexPosition = biconvexTransform.GetPosition();
    vec3f biconvexUp = biconvexTransform.GetUp();

    const float boundingSphereRadius = biconvex.GetWidth() * 0.5f;

    bool broadPhaseReject;
    StoneBoardRegion region = DetermineStoneBoardRegion( board, biconvexPosition, boundingSphereRadius, broadPhaseReject );

    const float thickness = board.GetThickness();

    if ( region == STONE_BOARD_REGION_Primary )
    {
        // common case: collision with primary surface of board only
        // no collision with edges or corners of board is possible

        vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,thickness) );

        vec3f local_stonePoint;
        vec3f local_stoneNormal;
        vec3f local_boardPoint;

        ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                                 biconvex, 
                                                 local_stonePoint,
                                                 local_stoneNormal,
                                                 local_boardPoint );

        stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
        stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
        boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
        boardNormal = vec3f(0,1,0);
    }
    else if ( region == STONE_BOARD_REGION_LeftSide )
    {
        // primary surface
        {
            vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,thickness) );

            vec3f local_stonePoint;
            vec3f local_stoneNormal;
            vec3f local_boardPoint;

            ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                                     biconvex, 
                                                     local_stonePoint,
                                                     local_stoneNormal,
                                                     local_boardPoint );

            stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
            stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
            boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
            boardNormal = vec3f(0,1,0);

            const float w = board.GetWidth() / 2;
            const float h = board.GetHeight() / 2;

            const float x = boardPoint.x();
            const float z = boardPoint.z();

            if ( x >= -w && x <= w && z >= -h && z <= h )
                return;
        }

        // left side plane
        {
            const float w = board.GetWidth() / 2;
            const float h = board.GetHeight() / 2;
            const float t = board.GetThickness();

            vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(-1,0,0,w) );

            vec3f local_stonePoint;
            vec3f local_stoneNormal;
            vec3f local_boardPoint;

            ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), plane.w(), 
                                                     biconvex, 
                                                     local_stonePoint,
                                                     local_stoneNormal,
                                                     local_boardPoint );

            stonePoint = TransformPoint( biconvexTransform.localToWorld, local_stonePoint );
            stoneNormal = TransformVector( biconvexTransform.localToWorld, local_stoneNormal );
            boardPoint = TransformPoint( biconvexTransform.localToWorld, local_boardPoint );
            boardNormal = vec3f(-1,0,0);

            const float y = boardPoint.y();
            const float z = boardPoint.z();

            if ( y <= t && z >= -h && z <= t )
                return;
        }

        // left side edge
        {
            const float w = board.GetWidth() / 2;
            const float h = board.GetHeight() / 2;
            const float t = board.GetThickness();

            const vec3f lineOrigin = vec3f( -w, t, -h );
            const vec3f lineDirection = vec3f(0,0,1);

            GetNearestPoint_Biconvex_Line( biconvex,
                                           biconvexPosition,
                                           biconvexUp,
                                           lineOrigin,
                                           lineDirection,
                                           stonePoint,
                                           boardPoint );

            const float x = boardPoint.x();
            const float y = boardPoint.y();
            const float z = boardPoint.z();

            const float dx = fabs( x - (-w) );
            const float dy = fabs( y - t );
            assert( dx < 0.001f );
            assert( dy < 0.001f );
            assert( z >= -h );
            assert( z <= h );

            vec3f local_point = transformPoint( biconvexTransform.worldToLocal, stonePoint );
            vec3f local_normal;

            GetBiconvexSurfaceNormalAtPoint_LocalSpace( local_point, biconvex, local_normal );

            stoneNormal = transformVector( biconvexTransform.localToWorld, local_normal );

            assert( fabs( length( stoneNormal ) - 1.0f ) < 0.001f );

            boardNormal = -stoneNormal;
        }
    }
    else
    {
        // not implemented yet!
    }
}

// -----------------------------------------------------------------------

bool StoneBoardCollision( const Biconvex & biconvex,
                          const Board & board, 
                          RigidBody & rigidBody,
                          StaticContact & contact )
{
    // detect collision with the board

    float depth;
    vec3f normal;
    if ( !IntersectStoneBoard( board, biconvex, RigidBodyTransform( rigidBody.position, rigidBody.orientation ), normal, depth ) )
        return false;

    // project the stone out of the board

    rigidBody.position += normal * depth;

    // fill the contact information for the caller

    vec3f stonePoint, stoneNormal, boardPoint, boardNormal;

    ClosestFeaturesStoneBoard( board, biconvex, 
                               RigidBodyTransform( rigidBody.position, rigidBody.orientation ), 
                               stonePoint, stoneNormal, boardPoint, boardNormal );

    contact.rigidBody = &rigidBody;
    contact.point = boardPoint;
    contact.normal = boardNormal;
    contact.depth = depth;

    return true;
}

bool StoneFloorCollision( const Biconvex & biconvex,
                          const Board & board, 
                          RigidBody & rigidBody,
                          StaticContact & contact )
{
    const float boundingSphereRadius = biconvex.GetBoundingSphereRadius();

    vec3f biconvexPosition = rigidBody.position;

    RigidBodyTransform biconvexTransform( rigidBody.position, rigidBody.orientation );

    float s1,s2;
    vec3f biconvexUp = biconvexTransform.GetUp();
    vec3f biconvexCenter = biconvexTransform.GetPosition();
    BiconvexSupport_WorldSpace( biconvex, biconvexCenter, biconvexUp, vec3f(0,1,0), s1, s2 );
    
    if ( s1 > 0 )
        return false;

    float depth = -s1;

    rigidBody.position += vec3f(0,depth,0);

    vec4f plane = TransformPlane( biconvexTransform.worldToLocal, vec4f(0,1,0,0) );

    vec3f local_stonePoint;
    vec3f local_stoneNormal;
    vec3f local_floorPoint;

    ClosestFeaturesBiconvexPlane_LocalSpace( vec3f( plane.x(), plane.y(), plane.z() ), 
                                             plane.w(), 
                                             biconvex, 
                                             local_stonePoint,
                                             local_stoneNormal,
                                             local_floorPoint );

    contact.rigidBody = &rigidBody;
    contact.point = TransformPoint( biconvexTransform.localToWorld, local_floorPoint );
    contact.normal = vec3f(0,1,0);
    contact.depth = depth;

    return true;
}

// -----------------------------------------------------------------------

#endif
