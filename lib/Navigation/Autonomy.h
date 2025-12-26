#ifndef AUTONOMY_H
#define AUTONOMY_H

#include <Arduino.h>
#include "config.h"

class Autonomy
{
public:
    Autonomy();

    // Inputs
    void update(float frontDistance, float rearDistance);

    // Outputs
    int getLeftSpeed() const;
    int getRightSpeed() const;
    NavigationState getNavState() const;
    
    // Command
    void reset();

private:
    float _frontDistance;
    float _rearDistance;
    
    int _leftSpeed;
    int _rightSpeed;
    NavigationState _navState;

    void updateLogic();
};

#endif
