/*
 * IMU.h
 *
 * Created: 3/28/2015 10:59:09 PM
 *  Author: ekt
 */ 


#ifndef IMU_H_
#define IMU_H_

class IMU {
public:
    void setup();
    void loop();

private:
    void check_reset_calibration_session();
    void compensate_sensor_errors();
    void reset_sensor_fusion();
    void read_sensors();

    // Sensors.cpp
    void I2C_Init();
    void Accel_Init();
    void Read_Accel();
    void Magn_Init();
    void Read_Magn();
    void Gyro_Init();
    void Read_Gyro();

    // DCM.cpp
    void Normalize(void);
    void Drift_correction(void);
    void Matrix_update(void);
    void Euler_angles(void);

    // Compass.cpp
    void Compass_Heading();


private:
    // Sensor variables
    float accel[3];  // Actually stores the NEGATED acceleration (equals gravity, if board not moving).
    float accel_min[3];
    float accel_max[3];

    float magnetom[3];
    float magnetom_min[3];
    float magnetom_max[3];
    float magnetom_tmp[3];

    float gyro[3];
    float gyro_average[3];
    int gyro_num_samples = 0;

    // DCM variables
    float MAG_Heading;
    float Accel_Vector[3]= {0, 0, 0}; // Store the acceleration in a vector
    float Gyro_Vector[3]= {0, 0, 0}; // Store the gyros turn rate in a vector
    float Omega_Vector[3]= {0, 0, 0}; // Corrected Gyro_Vector data
    float Omega_P[3]= {0, 0, 0}; // Omega Proportional correction
    float Omega_I[3]= {0, 0, 0}; // Omega Integrator
    float Omega[3]= {0, 0, 0};
    float errorRollPitch[3] = {0, 0, 0};
    float errorYaw[3] = {0, 0, 0};
    float DCM_Matrix[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float Update_Matrix[3][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}};
    float Temporary_Matrix[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    // Euler angles
    float yaw;
    float pitch;
    float roll;

    // DCM timing in the main loop
    unsigned long timestamp;
    unsigned long timestamp_old;
    float G_Dt; // Integration time for DCM algorithm

    // More output-state variables
    bool output_stream_on;
    bool output_single_on;
    int curr_calibration_sensor = 0;
    bool reset_calibration_session_flag = true;
    int num_accel_errors = 0;
    int num_magn_errors = 0;
    int num_gyro_errors = 0;
};



#endif /* IMU_H_ */