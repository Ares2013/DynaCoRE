#include "FootTask.hpp"
#include <Configuration.h>
#include <SagitP3_Controller/SagitP3_StateProvider.hpp>
#include <SagitP3/SagitP3_Model.hpp>
#include <SagitP3/SagitP3_Definition.h>

#include <Utils/utilities.hpp>

FootTask::FootTask(const RobotSystem* robot, int swing_foot):
    KinTask(4),
    robot_sys_(robot),
    swing_foot_(swing_foot)
{
    sp_ = SagitP3_StateProvider::getStateProvider();
    Jt_ = dynacore::Matrix::Zero(dim_task_, sagitP3::num_qdot);
    if(swing_foot_ == sagitP3_link::lAnkle) stance_foot_ = sagitP3_link::rAnkle;
    if(swing_foot_ == sagitP3_link::rAnkle) stance_foot_ = sagitP3_link::lAnkle;
    // printf("[BodyFoot Task] Constructed\n");
}

FootTask::~FootTask(){}

bool FootTask::_UpdateCommand(void* pos_des,
        const dynacore::Vector & vel_des,
        const dynacore::Vector & acc_des){
    dynacore::Vector* pos_cmd = (dynacore::Vector*)pos_des;
    
    dynacore::Vect3 foot_pos;
    dynacore::Quaternion foot_ori, foot_ori_des;
    robot_sys_->getPos(swing_foot_, foot_pos);
    robot_sys_->getOri(swing_foot_, foot_ori);

   foot_ori_des.x() = (*pos_cmd)[0]; 
   foot_ori_des.y() = (*pos_cmd)[1]; 
   foot_ori_des.z() = (*pos_cmd)[2]; 
   foot_ori_des.w() = (*pos_cmd)[3]; 

   dynacore::Quaternion ori_err = 
       dynacore::QuatMultiply(foot_ori_des, foot_ori.inverse());
   dynacore::Vect3 ori_err_so3;
   dynacore::convert(ori_err, ori_err_so3);
    pos_err_[0] = ori_err_so3[2];

    // TODO
    vel_des_[0] = 0.;
    acc_des_[0] = 0.;
    for(int i(1); i<dim_task_; ++i){
        pos_err_[i] = ((*pos_cmd)[i + 3] - foot_pos[i-1] );
        vel_des_[i] = vel_des[i];
        acc_des_[i] = acc_des[i];
    }

     //dynacore::pretty_print(pos_err_, std::cout, "pos err");
     //dynacore::pretty_print(vel_des, std::cout, "vel des");
     //dynacore::pretty_print(acc_des, std::cout, "acc des");

    return true;
}

bool FootTask::_UpdateTaskJacobian(){
    dynacore::Matrix Jswing, Jstance;
    robot_sys_->getFullJacobian(swing_foot_, Jswing);
    robot_sys_->getFullJacobian(stance_foot_, Jstance);
    //Jt_ = Jswing.block(3,0,3, sagitP3::num_qdot) - Jstance.block(3, 0, 3, sagitP3::num_qdot);
    Jt_= Jswing.block(2,0,4, sagitP3::num_qdot);
    (Jt_.block(0, 0, 4, sagitP3::num_virtual)).setZero();
    
    // dynacore::pretty_print(Jswing, std::cout, "Jswing");
    // dynacore::pretty_print(Jfoot, std::cout, "Jfoot");
    // dynacore::pretty_print(Jt_, std::cout, "Jt BodyFoot");
    return true;
}

bool FootTask::_UpdateTaskJDotQdot(){
    dynacore::Vector Jdotqdot;
    robot_sys_->getFullJDotQdot(swing_foot_, Jdotqdot);
    JtDotQdot_ = Jdotqdot.tail(dim_task_);

    return true;
}

