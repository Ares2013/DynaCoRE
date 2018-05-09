#ifndef WBDC_COM_HEIGHT_BODY_ORIENTATION_FOOT_TASK
#define WBDC_COM_HEIGHT_BODY_ORIENTATION_FOOT_TASK

#include <WBDC_Relax/WBDC_Relax_Task.hpp>

class Mercury_StateProvider;
class RobotSystem;

// CoM_{x, y, z}, BodyOri_{Rx, Ry, Rz}, Foot (x, y, z)
class CoMFootTask: public WBDC_Relax_Task{
public:
  CoMFootTask(RobotSystem*, int swing_foot);
  virtual ~CoMFootTask();

  dynacore::Vector Kp_vec_;
  dynacore::Vector Kd_vec_;

protected:
  int swing_foot_;

  // Update op_cmd_
  virtual bool _UpdateCommand(void* pos_des,
                              const dynacore::Vector & vel_des,
                              const dynacore::Vector & acc_des);
  // Update Jt_
  virtual bool _UpdateTaskJacobian();
  // Update JtDotQdot_
  virtual bool _UpdateTaskJDotQdot();

  const Mercury_StateProvider* sp_;
  const RobotSystem* robot_sys_;
};

#endif
