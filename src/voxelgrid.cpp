#include "voxelgrid.h"

#include <kdl/chainiksolverpos_lma.hpp>
#include <kdl/chainiksolverpos_nr.hpp>
#include <kdl/chainiksolvervel_pinv.hpp>

#include <Eigen/Dense>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
#include <algorithm>

class VoxelIndex {
public:
    VoxelIndex (int x, int y, int z) : x(x), y(y), z(z) {};

    int x, y, z;

	void print() {
		std::cout << "Voxel: " << x << "," << y << "," << z << std::endl;
	}
};

struct voxel_hash {
    size_t operator()(const VoxelIndex &k) const {
        size_t h1 = std::hash<int>()(k.x);
        size_t h2 = std::hash<int>()(k.y);
        size_t h3 = std::hash<int>()(k.z);
        return (h1 ^ (h2 << 1)) ^ h3;
    }
};

struct voxel_equal {
    bool operator()( const VoxelIndex& lhs, const VoxelIndex& rhs ) const {
        return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
    }
};

template <typename T> using VoxelMap = std::unordered_map<VoxelIndex, T, voxel_hash, voxel_equal>;

float jntarray_distance(const KDL::JntArray& a, const KDL::JntArray& b) {

	float dist = 0;

	for(unsigned int j=0; j<a.data.size(); j++) {
		dist += std::pow(a(j) - b(j), 2);
	}

	return dist;
}

void print_jntarray(const KDL::JntArray& a) {
	for(unsigned int j=0; j<a.data.size(); j++) {
		std::cout << a(j) << ",";
	}
	std::cout << std::endl;
}

inline void normalizeAngle(double& val, const double& min, const double& max) {
  if (val > max) {
    //Find actual angle offset
    double diffangle = fmod(val-max,2*M_PI);
    // Add that to upper bound and go back a full rotation
    val = max + diffangle - 2*M_PI;
  }

  if (val < min) {
    //Find actual angle offset
    double diffangle = fmod(min-val,2*M_PI);
    // Add that to upper bound and go back a full rotation
    val = min - diffangle + 2*M_PI;
  }
}

inline void normalizeAngle(double& val, const double& target) {
  if (val > target+M_PI) {
    //Find actual angle offset
    double diffangle = fmod(val-target,2*M_PI);
    // Add that to upper bound and go back a full rotation
    val = target + diffangle - 2*M_PI;
  }

  if (val < target-M_PI) {
    //Find actual angle offset
    double diffangle = fmod(target-val,2*M_PI);
    // Add that to upper bound and go back a full rotation
    val = target - diffangle + 2*M_PI;
  }
}

namespace manus
{
    namespace manipulator {

class VoxelGrid_internal {
public:

    std::default_random_engine generator;

	int voxel_size;
	KDL::ChainIkSolverVel_pinv iks;

    VoxelGrid_internal(const Chain& _chain, float voxel_resolution, int voxel_size): voxel_resolution(voxel_resolution), voxel_size(voxel_size), iks(_chain) {}
    ~VoxelGrid_internal() {}

    VoxelMap<std::vector<JntArray> > cache;
    float voxel_resolution;

	void cache_put(VoxelIndex i, JntArray q) {

        VoxelMap<std::vector<JntArray> >::iterator v = cache.find(i);

        if (v != cache.end()) {
            if (v->second.size() < voxel_size)
                v->second.push_back(q);
        } else {
            cache[i] = std::vector<JntArray> ();
            cache[i].push_back(q);
        }

	}

};



VoxelGrid::VoxelGrid(const Chain& _chain, const JntArray& _q_min, const JntArray& _q_max, double _eps, float voxel_resolution, int voxel_size):
    chain(_chain), q_min(_q_min), q_max(_q_max), fksolver(_chain), eps(_eps)
{

    assert(chain.getNrOfJoints()==_q_min.data.size());
    assert(chain.getNrOfJoints()==_q_max.data.size());

    _impl = new VoxelGrid_internal(_chain, voxel_resolution, voxel_size);

	//iksolver = new ChainIkSolverPos_LMA(_chain, L, _eps, 500, 0.0000001);
    //iksolver = new ChainIkSolverPos_LMA(_chain, L, _eps, 500, 0.0001);
	//iksolver = new ChainIkSolverPos_NR(_chain, fksolver, _impl->iks, 300, 1);

    for (uint i=0; i<chain.segments.size(); i++) {
        std::string type = chain.segments[i].getJoint().getTypeName();
        if (type.find("Rot")!=std::string::npos) {
            if (_q_max(types.size())>=std::numeric_limits<float>::max() &&
                    _q_min(types.size())<=std::numeric_limits<float>::lowest())
                types.push_back(BasicJointType::Continuous);
            else types.push_back(BasicJointType::RotJoint);
        }
        else if (type.find("Trans")!=std::string::npos)
            types.push_back(BasicJointType::TransJoint);

    }

    assert(types.size()==_q_max.data.size());
}


void VoxelGrid::precompute(int count) {

    for (int iter = 0; iter < count; iter++) {

        KDL::JntArray q_test(q_max.data.size());

        for(unsigned int j=0; j<q_max.data.size(); j++) {
            std::uniform_real_distribution<float> range (q_min(j), q_max(j));

            q_test(j) = range(_impl->generator);

        }

        fksolver.JntToCart(q_test,f);

        VoxelIndex voxel((int) (f.p.x() / _impl->voxel_resolution), (int) (f.p.y() / _impl->voxel_resolution), (int) (f.p.z() / _impl->voxel_resolution));
        _impl->cache_put(voxel, q_test);

    }

}


int VoxelGrid::CartToJnt(const KDL::JntArray &q_init, const KDL::Frame &p_in, KDL::JntArray &q_out, bool use_rotation) {

    q_out = q_init;
    VoxelIndex voxel((int) (p_in.p.x() / _impl->voxel_resolution), (int) (p_in.p.y() / _impl->voxel_resolution), (int) (p_in.p.z() / _impl->voxel_resolution));

    fksolver.JntToCart(q_out,f);
    delta_twist = diffRelative(p_in, f);

    if(Equal(delta_twist,Twist::Zero(),eps))
        return 1;

    const VoxelMap<std::vector<JntArray> >::iterator v = _impl->cache.find(voxel);

	float best_distance = std::numeric_limits<float>::max();

	KDL::JntArray q_test(q_min.data.size());

    if (v != _impl->cache.end()) {

		std::vector<size_t> idx(v->second.size());
		std::iota(idx.begin(), idx.end(), 0);
		std::sort(idx.begin(), idx.end(), [&v, &q_init](size_t i1, size_t i2) {return jntarray_distance(v->second[i1], q_init) < jntarray_distance(v->second[i2], q_init); });

	    for (std::vector<size_t>::iterator it = idx.begin(); it != idx.end(); it++) {
            Eigen::Matrix<double,6,1> L;
            L(0) = 1;
            L(1) = 1;
            L(2) = 1;
            L(3) = use_rotation ? 0.1 : 0;
            L(4) = use_rotation ? 0.1 : 0;
            L(5) = use_rotation ? 0.1 : 0;

            KDL::ChainIkSolverPos_LMA iksolver(chain, L, eps, 500, 0.0001);

	        int result = iksolver.CartToJnt(v->second[*it], p_in, q_test);
	        if (result < 0)
	            continue;
	        bool valid = true;
			//print_jntarray(q_test);
	        for(unsigned int j=0; j<q_min.data.size(); j++) {
	            if (types[j]==BasicJointType::Continuous)
	                continue;
	            if (types[j]==BasicJointType::TransJoint)
	                continue;
	            if (types[j]==BasicJointType::RotJoint)
					normalizeAngle(q_test(j), q_min(j), q_max(j));
	            if (q_test(j) < q_min(j) || q_test(j) > q_max(j))
					valid = false;
	        }

			float distance = jntarray_distance(q_test, q_init);

	        if (valid && distance < best_distance) {
				best_distance = distance;
				q_out = q_test;
				_impl->cache_put(voxel, q_out);
	        }
	    }
	}

	if (best_distance < std::numeric_limits<float>::max()) {
		_impl->cache_put(voxel, q_out);
        return 1;
	}

    return -3;
}

VoxelGrid::~VoxelGrid()
{

    delete _impl;
}


}

}