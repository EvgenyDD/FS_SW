#ifndef SERVO_H
#define SERVO_H

enum
{
	SERVO_EYE_R_FAR = 0,
	SERVO_EYE_R_NEAR,
	SERVO_EAR_R,
	SERVO_EAR_L,
	SERVO_EYE_L_NEAR,
	SERVO_EYE_L_FAR,
	SERVO_TONGUE,
	SERVO_COUNT,
};

static inline uint16_t servo_get_approximation(uint8_t servo, float adc)
{
	switch(servo)
	{
	case SERVO_EAR_R: return adc * 1.604f - 89.163f;
	case SERVO_EAR_L: return adc * 1.43534f + 3.56723f;
	case SERVO_EYE_L_NEAR: return adc * (1.42778f + 1.39f) * 0.5f + (14.043f - 0.9757f) * 0.5f;
	case SERVO_EYE_L_FAR: return adc * (1.409092f + 1.4254167f) * 0.5f + (10.18966f - 8.4515f) * 0.5f;
	case SERVO_EYE_R_FAR: return adc * (1.3478f + 1.52054f) * 0.5f + (24.697f - 53.6587f) * 0.5f;
	case SERVO_EYE_R_NEAR: return adc * (1.339815f + 1.3266f) * 0.5f + (27.574f + 6.81544f) * 0.5f;
	default: return 0;
	}
}

#endif // SERVO_H