#include "locator.h"
#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <iostream>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <boost/asio.hpp>
#include <thread>
#include <fstream>
#include <csignal>
#include <bits/stdc++.h>

#define PIN_TRIGGER 29 // GPIO21
#define PIN_ECHO 28 // GPIO20
#define PI 3.14159265
#define X_OFFSET 1
#define Y_OFFSET 2

using boost::asio::ip::tcp;
using namespace std;

int *globalIntensity;
int *globalAngle;
int *globalRange;
double globalXDistance;
double globalZDistance;

int fd;
int acclX, acclY, acclZ;
int gyroX, gyroY, gyroZ;
double acclX_scaled, acclY_scaled, acclZ_scaled;
double gyroX_scaled, gyroY_scaled, gyroZ_scaled;
double xRotation, yRotation;

fstream file;
fstream file1;

void Locator::tcpServer(double *x, double *y) {
    boost::asio::io_service io_service;

// acceptor object needs to be created to listen for new connections
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 13));
    tcp::socket socket(io_service);
    acceptor.accept(socket);
    double a, b;
    for (;;) {
        if (*x != a || *y != b) {
            boost::system::error_code ignored_error;
            std::string message = std::to_string(*x) + "," + std::to_string(*y);
            boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
            a = *x;
            b = *y;
        }
    }

}

void signalHandler(int signum) {
    cout << "Interrupt signal" << signum << "received.\n";

    file.close();
    file1.close();
    system("echo e >> /dev/ttyUSB0");
    exit(signum);
}

Locator::Locator() {
    //Ultrasonic setup:
    wiringPiSetup();
    pinMode(PIN_TRIGGER, OUTPUT);
    digitalWrite(PIN_TRIGGER, LOW);
    pinMode(PIN_ECHO, INPUT);
    //i2c setup:
    fd = wiringPiI2CSetup(0x68);
    int a = wiringPiI2CWriteReg8(fd, 0x6B, 0x00);//disable sleep mode
    printf("%d", a);
    printf("set 0x6B=%X\n", wiringPiI2CReadReg8(fd, 0x6B));
    //init tcp server:
    std::thread th(&Locator::tcpServer, this, &xRotation, &yRotation);
    th.detach();

    file.open("../log/intensity.log", ios::app);
    file1.open("../log/range.log", ios::app);

    signal(SIGINT, signalHandler);
}

void Locator::setRange(int *range) {
    globalRange = range;
}

void Locator::setIntensity(int *intensity) {
    globalIntensity = intensity;
}

void Locator::setAngle(int *angle) {
    globalAngle = angle;
}

int readI2c(int address) {
    int val;
    val = wiringPiI2CReadReg8(fd, address);
    val = val << 8;
    val += wiringPiI2CReadReg8(fd, address + 1);
    if (val >= 0x8000)
        val = -(65536 - val);

    return val;
}

double dist(double a, double b) {
    return sqrt((a * a) + (b * b));
}

double getYRotation(double x, double y, double z) {
    double radians;
    radians = atan2(x, dist(y, z));
    yRotation = -(radians * (180.0 / M_PI));
    return yRotation;
}

double getXRotation(double x, double y, double z) {
    double radians;
    radians = atan2(y, dist(x, z));
    xRotation = (radians * (180.0 / M_PI));
    return xRotation;
}

void getIMUData(double *x, double *y) {
    acclX = readI2c(0x3B);
    acclY = readI2c(0x3D);
    acclZ = readI2c(0x3F);

    acclX_scaled = acclX / 16384.0;
    acclY_scaled = acclY / 16384.0;
    acclZ_scaled = acclZ / 16384.0;

    *x = getXRotation(acclX_scaled, acclY_scaled, acclZ_scaled);
    *y = getYRotation(acclX_scaled, acclY_scaled, acclZ_scaled);
}

long getMicrotime() {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    return currentTime.tv_sec * (int) 1e6 + currentTime.tv_usec;
}

double getYDistance() {
    digitalWrite(PIN_TRIGGER, HIGH);
    usleep(10);
    digitalWrite(PIN_TRIGGER, LOW);

    int echo, previousEcho, lowHigh, highLow;
    long startTime, stopTime, difference;
    float rangeCm;
    lowHigh = highLow = echo = previousEcho = 0;
    while (0 == lowHigh || highLow == 0) {
        previousEcho = echo;
        echo = digitalRead(PIN_ECHO);
        if (0 == lowHigh && 0 == previousEcho && 1 == echo) {
            lowHigh = 1;
            startTime = getMicrotime();
        }
        if (1 == lowHigh && 1 == previousEcho && 0 == echo) {
            highLow = 1;
            stopTime = getMicrotime();
        }
    }
    difference = stopTime - startTime;
    rangeCm = difference / 58;
    double x, y;
    getIMUData(&x, &y);
    x = x >= 0 ? x : -x;
    rangeCm = cos(x * PI / 180) * rangeCm;
    return rangeCm;
}

void Locator::setXDistance(double distance) {
    double x, y;
    getIMUData(&x, &y);
    globalXDistance = cos(x * PI / 180) * distance;
}

void Locator::setZDistance(double distance) {
    double x, y;
    getIMUData(&x, &y);
    globalZDistance = cos(x * PI / 180) * cos(y * PI / 180) * distance;
}

void logIntensity(double x, double y, double z) {
    file << "[" << x << "," << y << "," << z << "]";
}

void logRange(double x, double y, double z) {
    file1 << "[" << x << "," << y << "," << z << "," << *globalAngle << "," << globalXDistance << "," << globalZDistance << "]";
}


void Locator::locate() {
    double xRotation, yRotation;
    double x, y, z;
    float yDistance = getYDistance();
    getIMUData(&xRotation, &yRotation);
    x = globalXDistance + (sin((*globalAngle - xRotation + X_OFFSET) * PI / 180) * *globalRange);
    y = yDistance + (sin((yRotation - Y_OFFSET) * PI / 180) * *globalRange);
    z = *globalRange * cos((*globalAngle - xRotation + X_OFFSET) * PI / 180) * cos((yRotation - Y_OFFSET) * PI / 180);
    z += globalZDistance;
    double intensity = ((380 - *globalRange) / 380.0) * *globalIntensity;
    logIntensity(x, y, intensity);
    logRange(x, y, z);
};
