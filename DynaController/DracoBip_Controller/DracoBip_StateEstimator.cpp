#include "DracoBip_StateEstimator.hpp"
#include "DracoBip_StateProvider.hpp"
#include <Utils/utilities.hpp>
#include <DracoBip/DracoBip_Model.hpp>
#include <DracoBip_Controller/DracoBip_DynaCtrl_Definition.h>

// Orientation Estimators
#include <DracoBip_Controller/StateEstimator/BasicAccumulation.hpp>

DracoBip_StateEstimator::DracoBip_StateEstimator(RobotSystem* robot):
    curr_config_(dracobip::num_q),
    curr_qdot_(dracobip::num_qdot)
{
    sp_ = DracoBip_StateProvider::getStateProvider();
    robot_sys_ = robot;
    ori_est_ = new BasicAccumulation();
}

DracoBip_StateEstimator::~DracoBip_StateEstimator(){
    delete ori_est_;
}

void DracoBip_StateEstimator::Initialization(DracoBip_SensorData* data){
    curr_config_.setZero();
    curr_qdot_.setZero();
    curr_config_[dracobip::num_qdot] = 1.;

    // Joint Set
    for (int i(0); i<dracobip::num_act_joint; ++i){
        curr_config_[dracobip::num_virtual + i] = data->jpos[i];
        curr_qdot_[dracobip::num_virtual + i] = data->jvel[i];
    }
    std::vector<double> imu_acc(3);
    std::vector<double> imu_ang_vel(3);

    for(int i(0); i<3; ++i){
        imu_acc[i]  = data->imu_acc[i];
        imu_ang_vel[i] = data->imu_ang_vel[i];
    }

    dynacore::Quaternion body_ori;
    body_ori.w() = 1.; body_ori.x() = 0.; body_ori.y() = 0.; body_ori.z() = 0;
    dynacore::Vect3 body_ang_vel;

    ori_est_->EstimatorInitialization(body_ori, imu_acc, imu_ang_vel);   
    ori_est_->getEstimatedState(body_ori, body_ang_vel);

    curr_config_[3] = body_ori.x();
    curr_config_[4] = body_ori.y();
    curr_config_[5] = body_ori.z();
    curr_config_[dracobip::num_qdot] = body_ori.w();

    for(int i(0); i<3; ++i)  curr_qdot_[i+3] = body_ang_vel[i];

    robot_sys_->UpdateSystem(curr_config_, curr_qdot_);

    dynacore::Vect3 foot_pos, foot_vel;
    robot_sys_->getPos(sp_->stance_foot_, foot_pos);
    robot_sys_->getLinearVel(sp_->stance_foot_, foot_vel);
    
    curr_config_[0] = -foot_pos[0];
    curr_config_[1] = -foot_pos[1];
    curr_config_[2] = -foot_pos[2];
    curr_qdot_[0] = -foot_vel[0];
    curr_qdot_[1] = -foot_vel[1];
    curr_qdot_[2] = -foot_vel[2];

    robot_sys_->UpdateSystem(curr_config_, curr_qdot_);

    sp_->Q_ = curr_config_;
    sp_->Qdot_ = curr_qdot_;

    // Right Contact 
    if(data->rfoot_contact) sp_->b_rfoot_contact_ = 1;
    else sp_->b_rfoot_contact_ = 0;
    // Left Contact 
    if(data->lfoot_contact) sp_->b_lfoot_contact_ = 1;
    else sp_->b_lfoot_contact_ = 0;
}

void DracoBip_StateEstimator::Update(DracoBip_SensorData* data){
    curr_config_.setZero();
    curr_qdot_.setZero();
    curr_config_[dracobip::num_qdot] = 1.;

    for (int i(0); i<dracobip::num_act_joint; ++i){
        curr_config_[dracobip::num_virtual + i] = data->jpos[i];
        curr_qdot_[dracobip::num_virtual + i] = data->jvel[i];
    }
    std::vector<double> imu_acc(3);
    std::vector<double> imu_ang_vel(3);
 
    for(int i(0); i<3; ++i){
        imu_acc[i]  = data->imu_acc[i];
        imu_ang_vel[i] = data->imu_ang_vel[i];
    }
   
    dynacore::Quaternion body_ori;
    body_ori.w() = 1.; body_ori.x() = 0.; body_ori.y() = 0.; body_ori.z() = 0;
    dynacore::Vect3 body_ang_vel;

    ori_est_->setSensorData( imu_acc, imu_ang_vel);
    ori_est_->getEstimatedState(body_ori, body_ang_vel);

    curr_config_[3] = body_ori.x();
    curr_config_[4] = body_ori.y();
    curr_config_[5] = body_ori.z();
    curr_config_[dracobip::num_qdot] = body_ori.w();

    for(int i(0); i<3; ++i)  curr_qdot_[i+3] = body_ang_vel[i];
    
    robot_sys_->UpdateSystem(curr_config_, curr_qdot_);

    // Foot position based offset
    dynacore::Vect3 foot_pos, foot_vel;
    robot_sys_->getPos(sp_->stance_foot_, foot_pos);
    robot_sys_->getLinearVel(sp_->stance_foot_, foot_vel);

    curr_config_[0] = -foot_pos[0];
    curr_config_[1] = -foot_pos[1];
    curr_config_[2] = -foot_pos[2];
    curr_qdot_[0] = -foot_vel[0];
    curr_qdot_[1] = -foot_vel[1];
    curr_qdot_[2] = -foot_vel[2];

    robot_sys_->UpdateSystem(curr_config_, curr_qdot_);

    sp_->Q_ = curr_config_;
    sp_->Qdot_ = curr_qdot_;

    //dynacore::pretty_print(sp_->Q_, std::cout, "state estimator config");

    // Right Contact 
    if(data->rfoot_contact) sp_->b_rfoot_contact_ = 1;
    else sp_->b_rfoot_contact_ = 0;
    // Left Contact 
    if(data->lfoot_contact) sp_->b_lfoot_contact_ = 1;
    else sp_->b_lfoot_contact_ = 0;
}
