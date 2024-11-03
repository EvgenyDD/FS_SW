#ifndef HELPER_H
#define HELPER_H

inline float map_float(float x, float in_min, float in_max, float out_min, float out_max)
{
    if(x < in_min)
    {
        x = in_min;
    }
    if(x > in_max)
    {
        x = in_max;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#endif // HELPER_H