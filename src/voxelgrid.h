
#ifndef VOXELGRIDCHAINSOLVER_TL_HPP
#define VOXELGRIDCHAINSOLVER_TL_HPP

#include <kdl/chainfksolverpos_recursive.hpp>
#include <kdl/chainiksolver.hpp>

using namespace KDL;

namespace manus {
namespace manipulator {

enum BasicJointType { RotJoint, TransJoint, Continuous };

class VoxelGrid_internal;

class VoxelGrid
{
public:
    VoxelGrid(const Chain& chain,const JntArray& q_min, const JntArray& q_max, double eps=1e-3, float voxel_resolution = 20.0, int voxel_size = 20);

    ~VoxelGrid();

    int CartToJnt(const KDL::JntArray& q_init, const KDL::Frame& p_in, KDL::JntArray& q_out, bool use_rotation = true);

    void precompute(int count);

private:
    const Chain chain;
    JntArray q_min;
    JntArray q_max;

    KDL::Twist bounds;

	//KDL::ChainIkSolverPos * iksolver;
    KDL::ChainFkSolverPos_recursive fksolver;

    double maxtime;
    double eps;

    std::vector<BasicJointType> types;

    Frame f;
    Twist delta_twist;

    inline static double fRand(double min, double max)
    {
        double f = (double)rand() / RAND_MAX;
        return min + f * (max - min);
    }

    VoxelGrid_internal *_impl;

};

/**
 * determines the rotation axis necessary to rotate from frame b1 to the
 * orientation of frame b2 and the vector necessary to translate the origin
 * of b1 to the origin of b2, and stores the result in a Twist
 * datastructure.  The result is w.r.t. frame b1.
 * \param F_a_b1 frame b1 expressed with respect to some frame a.
 * \param F_a_b2 frame b2 expressed with respect to some frame a.
 * \warning The result is not a real Twist!
 * \warning In contrast to standard KDL diff methods, the result of
 * diffRelative is w.r.t. frame b1 instead of frame a.
 */
IMETHOD Twist diffRelative(const Frame & F_a_b1, const Frame & F_a_b2, double dt = 1)
{
    return Twist(F_a_b1.M.Inverse() * diff(F_a_b1.p, F_a_b2.p, dt),
                 F_a_b1.M.Inverse() * diff(F_a_b1.M, F_a_b2.M, dt));
}

}

}

#endif
