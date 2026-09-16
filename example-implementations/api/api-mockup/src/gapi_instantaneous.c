/**
 * @file gapi_instantaneous.c
 * @brief Definition file for API instantaneous data sending
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_instantaneous.h"
#include "pb_decode.h"

enum { SEC_IN_US = 1000000 };

static void set_phase(GeisaTypeInstantaneousQuantities_PerPhase *phase,
		      GeisaTypePhase phase_type, int64_t current,
		      int64_t voltage, int64_t active_power,
		      int64_t reactive_power, int64_t apparent_power,
		      double voltage_angle, double current_angle,
		      double power_factor, int64_t fundamental_current,
		      int64_t fundamental_voltage, double current_thd,
		      double voltage_thd, double power_factor_angle,
		      double current_tdd, int64_t distortion_current,
		      double voltage_harmonic)
{
	*phase = (GeisaTypeInstantaneousQuantities_PerPhase)
		GeisaTypeInstantaneousQuantities_PerPhase_init_default;
	phase->message_version = 1;
	phase->phase = phase_type;
	phase->has_current_micro_a = true;
	phase->current_micro_a = current;
	phase->has_voltage_micro_v = true;
	phase->voltage_micro_v = voltage;
	phase->has_active_power_micro_w_sum = true;
	phase->active_power_micro_w_sum = active_power;
	phase->has_reactive_power_micro_var_sum = true;
	phase->reactive_power_micro_var_sum = reactive_power;
	phase->has_apparent_power_micro_va_sum = true;
	phase->apparent_power_micro_va_sum = apparent_power;
	phase->has_voltage_angle_deg = true;
	phase->voltage_angle_deg = voltage_angle;
	phase->has_current_angle_deg = true;
	phase->current_angle_deg = current_angle;
	phase->has_power_factor = true;
	phase->power_factor = power_factor;
	phase->has_current_micro_a_fundamental = true;
	phase->current_micro_a_fundamental = fundamental_current;
	phase->has_voltage_micro_v_fundamental = true;
	phase->voltage_micro_v_fundamental = fundamental_voltage;
	phase->has_current_thd_percent = true;
	phase->current_thd_percent = current_thd;
	phase->has_voltage_thd_percent = true;
	phase->voltage_thd_percent = voltage_thd;
	phase->has_power_factor_angle_deg = true;
	phase->power_factor_angle_deg = power_factor_angle;
	phase->has_current_tdd_percent = true;
	phase->current_tdd_percent = current_tdd;
	phase->has_current_distortion_micro_a_rms = true;
	phase->current_distortion_micro_a_rms = distortion_current;
	phase->has_voltage_2nd_harmonic_percent = true;
	phase->voltage_2nd_harmonic_percent = voltage_harmonic;
}

static void geisa_get_instantaneous_data(GeisaInstantaneousQuantities *response)
{
	uint64_t timestamp = time(NULL) * SEC_IN_US;

	*response = (GeisaInstantaneousQuantities)
		GeisaInstantaneousQuantities_init_default;
	response->message_version = 1;
	response->timestamp_us = timestamp;
	// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
	set_phase(&response->phase_a, GeisaTypePhase_PHASE_A, 10, 120, 1200,
		  300, 1300, 10, 30, 0.95, 9, 115, 5, 4, 20, 3, 1, 1);
	response->has_phase_a = true;
	set_phase(&response->phase_b, GeisaTypePhase_PHASE_B, 11, 121, 1210,
		  310, 1310, 11, 31, 0.96, 10, 116, 6, 5, 21, 4, 1, 1.1);
	response->has_phase_b = true;
	set_phase(&response->phase_c, GeisaTypePhase_PHASE_C, 12, 122, 1220,
		  320, 1320, 12, 32, 0.97, 11, 117, 7, 6, 22, 5, 1, 1.2);
	response->has_phase_c = true;
	set_phase(&response->phase_n, GeisaTypePhase_PHASE_N, 5, 60, 600, 150,
		  650, 5, 15, 0.98, 4, 55, 2, 1.5, 10, 1, 1, 0.5);
	response->has_phase_n = true;
	response->has_other = true;
	response->other.message_version = 1;
	response->other.timestamp_us = timestamp;
	response->other.has_neutral_current_imputed_micro_a = true;
	response->other.neutral_current_imputed_micro_a = 5;
	response->other.has_load_side_voltage_micro_v = true;
	response->other.load_side_voltage_micro_v = 123;
	response->has_frequency_hz = true;
	response->frequency_hz = 60.0;
	response->has_temperature_celsius = true;
	response->temperature_celsius = 25.0;
	// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
}

static void *gapi_instantaneous_thread(void *arg)
{
	struct mosquitto *mosq = (struct mosquitto *)arg;
	GeisaInstantaneousQuantities response =
		GeisaInstantaneousQuantities_init_default;
	size_t encoded_size = 0;
	uint8_t *message = NULL;
	pb_ostream_t ostream;
	bool status = true;

	while (running) {
		geisa_get_instantaneous_data(&response);
		status = pb_get_encoded_size(
			&encoded_size, GeisaInstantaneousQuantities_fields,
			&response);
		if (!status) {
			fprintf(stderr, "[Instantaneous] Error calculating "
					"size of instantaneous data\n");
			sleep(1);
			continue;
		}
		message = malloc(encoded_size);
		if (message == NULL) {
			fprintf(stderr, "[Instantaneous] Failed to allocate "
					"memory for instantaneous data\n");
			sleep(1);
			continue;
		}
		ostream = pb_ostream_from_buffer(message, encoded_size);
		status =
			pb_encode(&ostream, GeisaInstantaneousQuantities_fields,
				  &response);
		if (!status) {
			fprintf(stderr, "[Instantaneous] Error encoding "
					"instantaneous data\n");
			free(message);
			sleep(1);
			continue;
		}
		api_publish(mosq, "geisa/api/instantaneous/data", encoded_size,
			    message, 0);
		free(message);
		sleep(1);
	}

	return NULL;
}

int api_instantaneous_init(pthread_t *thread, struct mosquitto *mosq)
{
	int ret = 0;
	ret = pthread_create(thread, NULL, gapi_instantaneous_thread, mosq);
	if (ret != 0) {
		fprintf(stderr, "[Instantaneous] Failed to create "
				"instantaneous thread\n");
		return ret;
	}
	return 0;
}
