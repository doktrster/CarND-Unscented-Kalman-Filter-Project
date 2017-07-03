#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;


  n_x_ = 5;

  n_aug_ = 7;


  // initial state vector
  x_ = VectorXd(n_x_);

  // initial covariance matrix
  P_ = Identity(n_x_, n_x_);

  cout << "Matrix P:\n\n" << P_ << endl;

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 9;  // PARAMETER FOR TUNUNG

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 3;  .//PARAMETER FOR TUNUNG

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */


  is_initialized_ = false;

  lambda_ = 3 - n_x_;

  weights_ = = VectorXd(2*n_aug_+1);

}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package)
{
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
  double m0 = meas_package.raw_measurements_(0);
  double m1 = meas_package.raw_measurements_(1);

  if(!is_initialized_) 
  {


    // Initialisation

    if(meas_package.sensor_type_ == MeasurementPackage::LASER) 
    {
      x_ << m0, m1, 0, 0, 0;
    }
    else
    {
      x_ << m0 * cos(m1), m0 * sin(m1), 0, 0, 0;
    }

    prev_timestamp_ = meas_package.timestamp_;
    is_initialized_ = true;
    
    return;
  }


    // Prediction

    long delta_t = (meas_package.timestamp_ - prev_timestamp_) / 1000000.0;
    prev_timestamp_ = meas_package.timestamp_;

    Prediction(delta_t);


    // Update

  if (meas_package.sensor_type_ == MeasurementPackage::LASER) 
  {
    UpdateLidar(meas_package);
  } 
  else 
  {
    UpdateRadar(meas_package);
  }

  cout << "x_: " << x_ << endl;

  cout << "P_:\n" << P_ << endl;

}




/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t)
{
  /**
  TODO:
  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
  
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
  P_aug.fill(0.0);
  P_aug.topLeftCorner(n_x_, n_x_) = P_;

  MatrixXd Q = MatrixXd(2, 2);

  double std_a_2 = pow(std_a_, 2);
  double std_yawdd_2 = pow(std_yawdd_, 2);

  Q << std_a_2, 0,
       0, std_yawdd_2;

  P_aug.bottomRightCorner(2, 2) = Q;

  
  // square root from matrix P_aug
  MatrixXd A = P_aug.llt().matrixL();


  VectorXd x_aug = VectorXd(n_aug_);

  x_aug.head(5) = x_;

  x_aug(n_x_) = 0;
  x_aug(n_x_ + 1) = 0;


  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
  Xsig_aug.col(0)  = x_aug;

  for (int i = 0; i < n_aug_; i++)
  {
    Xsig_aug.col(i + 1) = x_aug + sqrt(lambda_ + n_aug_) * A.col(i);
    Xsig_aug.col(i + n_aug_ + 1) = x_aug - sqrt(lambda_ + n_aug_) * A.col(i);
  }

  
  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  {

      double p_x = Xsig_aug(0,i);
      double p_y = Xsig_aug(1,i);
      double v = Xsig_aug(2,i);
      double yaw = Xsig_aug(3,i);
      double yawd = Xsig_aug(4,i);
      double nu_a = Xsig_aug(5,i);
      double nu_yawdd = Xsig_aug(6,i);
      double px_p;
      double py_p;
      

      if (fabs(yawd) > 0.001)
      {
          px_p = p_x + v / yawd * (sin(yaw + yawd * delta_t) - sin(yaw));
          py_p = p_y + v / yawd * (-cos(yaw + yawd * delta_t) + cos(yaw));
      }
      else
      {
          px_p = p_x + v * cos(yaw) * delta_t;
          py_p = p_y + v * sin(yaw) * delta_t;
      }
      
      double v_p = v;
      double yaw_p = yaw + yawd * delta_t;
      double yawd_p = yawd;
      
      px_p = px_p + 0.5 * nu_a * pow(delta_t, 2) * cos(yaw);
      py_p = py_p + 0.5 * nu_a * pow(delta_t, 2) * sin(yaw);
      v_p = v_p + nu_a * delta_t;


      yaw_p = yaw_p + 0.5 * pow(delta_t, 2) * nu_yawdd;
      yawd_p = yawd_p + delta_t * nu_yawdd;
      
      Xsig_pred_(0,i) = px_p;
      Xsig_pred_(1,i) = py_p;
      Xsig_pred_(2,i) = v_p;
      Xsig_pred_(3,i) = yaw_p;
      Xsig_pred_(4,i) = yawd_p;

      }
  


  x_.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1; i++) 
  {
    x_ = x_ + weights_(i) * Xsig_pred_.col(i);
  }

  P_.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1; i++) 
  {
    
    VectorXd x_diff = Xsig_pred_.col(i) - x_;

    while(x_diff(3) > M_PI)
    {
      x_diff(3) -= 2.0 * M_PI;
    }
    while(x_diff(3) < -M_PI) 
    {
      x_diff(3) += 2.0 * M_PI;
    }

    P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
  }

}





/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package)
{
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */

  int n_z = 2;

  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

  VectorXd z_pred = VectorXd(n_z);
  
  MatrixXd S = MatrixXd(n_z,n_z);
  
  for (int i = 0; i < 2 * n_aug_ + 1; i++) 
  { 
      Zsig(0,i) = Xsig_pred_(0,i);
      Zsig(1,i) = Xsig_pred_(1,i);
  }

  z_pred.fill(0.0);
  for (int i=0; i < 2 * n_aug_+1; i++) 
  {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }


  S.fill(0.0);
  for(int i = 0; i < 2 * n_aug_ + 1; i++) 
  {
    VectorXd z_diff = Zsig.col(i) - z_pred;

    while(z_diff(1) > M_PI)
    {
    z_diff(1) -= 2.0 * M_PI;
    }
    while(z_diff(1) < -M_PI)
    {
      z_diff(1) += 2.0 * M_PI;
    }

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  
  MatrixXd R = MatrixXd(n_z,n_z);
  R << pow(std_laspx_, 2), 0,
       0, pow(std_laspy_, 2);
  S = S + R;

  MatrixXd Tc = MatrixXd(n_x_, n_z);
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) 
  {

    VectorXd z_diff = Zsig.col(i) - z_pred;

    while(z_diff(1) > M_PI)
    {
      z_diff(1) -= 2.0 * M_PI;
    }
    while(z_diff(1) < -M_PI)
    {
      z_diff(1) += 2.0 * M_PI;
    }

    VectorXd x_diff = Xsig_pred_.col(i) - x_;

    while(x_diff(3) > M_PI)
    {
      x_diff(3) -= 2.0 * M_PI;
    }
    while(x_diff(3) < -M_PI)
    {
      x_diff(3) += 2.0 * M_PI;
    }

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  MatrixXd K = Tc * S.inverse();

  VectorXd z = meas_package.raw_measurements_;
  VectorXd z_diff = z - z_pred;

  while(z_diff(1) > M_PI)
  {
    z_diff(1) -= 2.0 * M_PI;
  }
  while(z_diff(1) < -M_PI)
  {
    z_diff(1) += 2.0 * M_PI;
  }

  NIS_laser_ = NIS(z_diff, S);

  x_ = x_ + K * z_diff;
  MatrixXd Kt = K.transpose();
  P_ = P_ - K * S * Kt;

  while (x_(3) > M_PI)
  {
    x_(3) -= 2.0 * M_PI;
  }
  while (x_(3) <- M_PI)
  {
    x_(3)+=2.0 * M_PI;
  }

}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package)
{
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */

  int n_z = 3;

  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

  VectorXd z_pred = VectorXd(n_z);
  
  MatrixXd S = MatrixXd(n_z,n_z);


  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  { 
      double p_x = Xsig_pred_(0, i);
      double p_y = Xsig_pred_(1, i);
      double v = Xsig_pred_(2, i);
      double yaw = Xsig_pred_(3, i);

      double v1 = cos(yaw) * v;
      double v2 = sin(yaw) * v;


      Zsig(0, i) = sqrt(pow(p_x, 2) + pow(p_y, 2)); //r
      Zsig(1, i) = atan2(p_y, p_x); // phi
      Zsig(2, i) = (p_x * v1 + p_y * v2) / sqrt(pow(p_x, 2) + pow(p_y, 2)); //r_dot
  }


  z_pred.fill(0.0);

  for (int i = 0; i < 2 * n_aug_ + 1; i++)
  {
      z_pred += weights_(i) * Zsig.col(i);
  }


  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {

    VectorXd z_diff = Zsig.col(i) - z_pred;

    while(z_diff(1) > M_PI)
    {
      z_diff(1) -= 2.0 * M_PI;
    }
    while(z_diff(1) < -M_PI)
    {
      z_diff(1) += 2.0 * M_PI;
    }

    S += weights_(i) * z_diff * z_diff.transpose();
  }

  MatrixXd R = MatrixXd(n_z, n_z);

  R << pow(std_radr_, 2), 0, 0,
       0, pow(std_radphi_, 2), 0,
       0, 0, pow(std_radrd_, 2);

  S += R;

  MatrixXd Tc = MatrixXd(n_x_, n_z);
  Tc.fill(0.0);

  for(int i = 0; i < 2 * n_aug_ + 1; i++) 
  {

    VectorXd z_diff = Zsig.col(i) - z_pred;

    while(z_diff(1) > M_PI)
    {
      z_diff(1) -= 2.0 * M_PI;
    }
    while(z_diff(1) < -M_PI)
    {
      z_diff(1) += 2.0 * M_PI;
    }


    VectorXd x_diff = Xsig_pred_.col(i) - x_;
  
    while(x_diff(3) > M_PI)
    {
      x_diff(3) -= 2.0 * M_PI;
    }
    while(x_diff(3) <- M_PI)
    {
      x_diff(3) += 2.0 * M_PI;
    }

    Tc += weights_(i) * x_diff * z_diff.transpose();
  }
  MatrixXd K = Tc * S.inverse();

  VectorXd z = meas_package.raw_measurements_;
  VectorXd z_diff = z - z_pred;

  while(z_diff(1) > M_PI)
  {
    z_diff(1) -= 2.0 * M_PI;
  }
  while(z_diff(1) <- M_PI)
  {
    z_diff(1) += 2.0 * M_PI;
  }

  NIS_radar_ = NIS(z_diff, S);

  
  x_ = x_ + K * z_diff;

  MatrixXd Kt = K.transpose();
  P_ -= K * S * Kt;

  while(x_(3) > M_PI)
  {
    x_(3) -= 2.0 * M_PI;
  }
  while(x_(3) < -M_PI)
  {
    x_(3) += 2.0 * M_PI;
  }
}
