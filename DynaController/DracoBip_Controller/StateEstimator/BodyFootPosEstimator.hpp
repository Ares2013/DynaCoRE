#ifndef BODY_FOOT_POSITON_ESTIMATOR_DRACO_BIPED
#define BODY_FOOT_POSITON_ESTIMATOR_DRACO_BIPED

#include <Utils/wrap_eigen.hpp>
#include <Filter/filters.hpp>

class MoCapManager;
class RobotSystem;
class DracoBip_StateProvider;

class BodyFootPosEstimator{
    public:
        BodyFootPosEstimator(const RobotSystem*);
        ~BodyFootPosEstimator();

        void Initialization(const dynacore::Quaternion & body_ori);
        void Update();

        void getMoCapBodyOri(dynacore::Quaternion & quat);
        void getMoCapBodyVel(dynacore::Vect3 & body_vel);
        void getMoCapBodyPos(const dynacore::Quaternion& body_ori, 
                dynacore::Vect3 & local_body_pos);

    protected:
        MoCapManager* mocap_manager_;
        DracoBip_StateProvider* sp_;

        std::vector<filter*> vel_filter_;
        dynacore::Vect3 body_led_vel_;

        const RobotSystem* robot_sys_;
};

#endif
