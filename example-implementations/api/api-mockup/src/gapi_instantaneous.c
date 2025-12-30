/**
 * @file gapi_instantaneous.c
 * @brief Definition file for API instantaneous data sending
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_instantaneous.h"

static void
geisa_get_instantaneous_data(InstantaneousQuantities *response,
			     TypeInstantaneousQuantitiesPerPhase *phase_a,
			     TypeInstantaneousQuantitiesPerPhase *phase_b,
			     TypeInstantaneousQuantitiesPerPhase *phase_c,
			     TypeInstantaneousQuantitiesPerPhase *phase_n,
			     TypeInstantaneousQuantitiesOther *other)
{
	// This function should interact with the GEISA system to retrieve
	// instantaneous data. Here we provide a mock implementation.
	instantaneous_quantities__init(response);
	type_instantaneous_quantities__per_phase__init(phase_a);
	type_instantaneous_quantities__per_phase__init(phase_b);
	type_instantaneous_quantities__per_phase__init(phase_c);
	type_instantaneous_quantities__per_phase__init(phase_n);
	type_instantaneous_quantities__other__init(other);

	time_t timestamp = time(NULL) * SEC_IN_MS;

	// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
	// Mock data assignment
	(*phase_a).message_version = 1;
	(*phase_a).phase = TYPE_PHASE__PHASE_A;
	(*phase_a).microamps = 10.0F;
	(*phase_a).microvolts = 120.0F;
	(*phase_a).microw = 1200.0F;
	(*phase_a).microvar = 300.0F;
	(*phase_a).microva = 1300.0F;
	(*phase_a).voltage_angle = 10.0F;
	(*phase_a).current_angle = 30.0F;
	(*phase_a).pf = 0.95F;
	(*phase_a).microamps_fundamental = 9.5F;
	(*phase_a).microvolts_fundamental = 115.0F;
	(*phase_a).current_percentage_thd = 5.0F;
	(*phase_a).voltage_percentage_thd = 4.0F;
	(*phase_a).pf_angle = 20.0F;
	(*phase_a).current_percentage_tdd = 3.0F;
	(*phase_a).harmonic_current = 0.5F;
	(*phase_a).phase_voltage_percentage_2nd_harmonic = 1.0F;

	(*phase_b).message_version = 1;
	(*phase_b).phase = TYPE_PHASE__PHASE_B;
	(*phase_b).microamps = 11.0F;
	(*phase_b).microvolts = 121.0F;
	(*phase_b).microw = 1210.0F;
	(*phase_b).microvar = 310.0F;
	(*phase_b).microva = 1310.0F;
	(*phase_b).voltage_angle = 11.0F;
	(*phase_b).current_angle = 31.0F;
	(*phase_b).pf = 0.96F;
	(*phase_b).microamps_fundamental = 10.5F;
	(*phase_b).microvolts_fundamental = 116.0F;
	(*phase_b).current_percentage_thd = 6.0F;
	(*phase_b).voltage_percentage_thd = 5.0F;
	(*phase_b).pf_angle = 21.0F;
	(*phase_b).current_percentage_tdd = 4.0F;
	(*phase_b).harmonic_current = 0.6F;
	(*phase_b).phase_voltage_percentage_2nd_harmonic = 1.1F;

	(*phase_c).message_version = 1;
	(*phase_c).phase = TYPE_PHASE__PHASE_C;
	(*phase_c).microamps = 12.0F;
	(*phase_c).microvolts = 122.0F;
	(*phase_c).microw = 1220.0F;
	(*phase_c).microvar = 320.0F;
	(*phase_c).microva = 1320.0F;
	(*phase_c).voltage_angle = 12.0F;
	(*phase_c).current_angle = 32.0F;
	(*phase_c).pf = 0.97F;
	(*phase_c).microamps_fundamental = 11.5F;
	(*phase_c).microvolts_fundamental = 117.0F;
	(*phase_c).current_percentage_thd = 7.0F;
	(*phase_c).voltage_percentage_thd = 6.0F;
	(*phase_c).pf_angle = 22.0F;
	(*phase_c).current_percentage_tdd = 5.0F;
	(*phase_c).harmonic_current = 0.7F;
	(*phase_c).phase_voltage_percentage_2nd_harmonic = 1.2F;

	(*phase_n).message_version = 1;
	(*phase_n).microamps = 5.0F;
	(*phase_n).microvolts = 60.0F;
	(*phase_n).microw = 600.0F;
	(*phase_n).microvar = 150.0F;
	(*phase_n).microva = 650.0F;
	(*phase_n).voltage_angle = 5.0F;
	(*phase_n).current_angle = 15.0F;
	(*phase_n).pf = 0.98F;
	(*phase_n).microamps_fundamental = 4.5F;
	(*phase_n).microvolts_fundamental = 55.0F;
	(*phase_n).current_percentage_thd = 2.0F;
	(*phase_n).voltage_percentage_thd = 1.5F;
	(*phase_n).pf_angle = 10.0F;
	(*phase_n).current_percentage_tdd = 1.0F;
	(*phase_n).harmonic_current = 0.2F;
	(*phase_n).phase_voltage_percentage_2nd_harmonic = 0.5F;

	(*other).message_version = 1;
	(*other).timestamp = timestamp;
	(*other).neutral_imputed_microamps = 5.5F;
	(*other).load_side_microvolts = 123.0F;

	// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

	response->timestamp = timestamp;
	response->phase_a = phase_a;
	response->phase_b = phase_b;
	response->phase_c = phase_c;
	response->phase_n = phase_n;
	response->other = other;
}

static void *gapi_instantaneous_thread(void *arg)
{
	struct mosquitto *mosq = (struct mosquitto *)arg;
	TypeInstantaneousQuantitiesPerPhase phase_a;
	TypeInstantaneousQuantitiesPerPhase phase_b;
	TypeInstantaneousQuantitiesPerPhase phase_c;
	TypeInstantaneousQuantitiesPerPhase phase_n;
	TypeInstantaneousQuantitiesOther other;
	InstantaneousQuantities response;
	uint8_t *message = NULL;

	while (running) {
		geisa_get_instantaneous_data(&response, &phase_a, &phase_b,
					     &phase_c, &phase_n, &other);
		message = malloc(
			instantaneous_quantities__get_packed_size(&response));
		if (message == NULL) {
			fprintf(stderr, "[Instantaneous] Failed to allocate "
					"memory for instantaneous data\n");
			sleep(1);
			continue;
		}
		instantaneous_quantities__pack(&response, message);
		api_publish(
			mosq, "geisa/api/instantaneous-data",
			instantaneous_quantities__get_packed_size(&response),
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
