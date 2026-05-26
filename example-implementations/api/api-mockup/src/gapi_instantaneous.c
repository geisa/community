/**
 * @file gapi_instantaneous.c
 * @brief Definition file for API instantaneous data sending
 * @copyright Copyright (C) 2025 Southern California Edison
 */

#include "gapi_instantaneous.h"
#include "pb_decode.h"

static void geisa_get_instantaneous_data(GeisaInstantaneousQuantities *response)
{
	// This function should interact with the GEISA system to retrieve
	// instantaneous data. Here we provide a mock implementation.
	GeisaTypeInstantaneousQuantities_PerPhase phase_a;
	GeisaTypeInstantaneousQuantities_PerPhase phase_b;
	GeisaTypeInstantaneousQuantities_PerPhase phase_c;
	GeisaTypeInstantaneousQuantities_PerPhase phase_n;
	GeisaTypeInstantaneousQuantities_Other other;

	time_t timestamp = time(NULL) * SEC_IN_MS;

	// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
	// Mock data assignment
	phase_a.message_version = 1;
	phase_a.phase = GeisaTypePhase_PHASE_A;
	phase_a.microAmps = 10.0F;
	phase_a.microVolts = 120.0F;
	phase_a.microW = 1200.0F;
	phase_a.microVAR = 300.0F;
	phase_a.microVA = 1300.0F;
	phase_a.Voltage_Angle = 10.0F;
	phase_a.Current_Angle = 30.0F;
	phase_a.PF = 0.95F;
	phase_a.microAmps_fundamental = 9.5F;
	phase_a.microVolts_fundamental = 115.0F;
	phase_a.Current_percentage_THD = 5.0F;
	phase_a.Voltage_percentage_THD = 4.0F;
	phase_a.PF_Angle = 20.0F;
	phase_a.Current_percentage_TDD = 3.0F;
	phase_a.Harmonic_Current = 0.5F;
	phase_a.Phase_Voltage_percentage_2nd_Harmonic = 1.0F;

	phase_b.message_version = 1;
	phase_b.phase = GeisaTypePhase_PHASE_B;
	phase_b.microAmps = 11.0F;
	phase_b.microVolts = 121.0F;
	phase_b.microW = 1210.0F;
	phase_b.microVAR = 310.0F;
	phase_b.microVA = 1310.0F;
	phase_b.Voltage_Angle = 11.0F;
	phase_b.Current_Angle = 31.0F;
	phase_b.PF = 0.96F;
	phase_b.microAmps_fundamental = 10.5F;
	phase_b.microVolts_fundamental = 116.0F;
	phase_b.Current_percentage_THD = 6.0F;
	phase_b.Voltage_percentage_THD = 5.0F;
	phase_b.PF_Angle = 21.0F;
	phase_b.Current_percentage_TDD = 4.0F;
	phase_b.Harmonic_Current = 0.6F;
	phase_b.Phase_Voltage_percentage_2nd_Harmonic = 1.1F;

	phase_c.message_version = 1;
	phase_c.phase = GeisaTypePhase_PHASE_C;
	phase_c.microAmps = 12.0F;
	phase_c.microVolts = 122.0F;
	phase_c.microW = 1220.0F;
	phase_c.microVAR = 320.0F;
	phase_c.microVA = 1320.0F;
	phase_c.Voltage_Angle = 12.0F;
	phase_c.Current_Angle = 32.0F;
	phase_c.PF = 0.97F;
	phase_c.microAmps_fundamental = 11.5F;
	phase_c.microVolts_fundamental = 117.0F;
	phase_c.Current_percentage_THD = 7.0F;
	phase_c.Voltage_percentage_THD = 6.0F;
	phase_c.PF_Angle = 22.0F;
	phase_c.Current_percentage_TDD = 5.0F;
	phase_c.Harmonic_Current = 0.7F;
	phase_c.Phase_Voltage_percentage_2nd_Harmonic = 1.2F;

	phase_n.message_version = 1;
	phase_n.phase = GeisaTypePhase_PHASE_N;
	phase_n.microAmps = 5.0F;
	phase_n.microVolts = 60.0F;
	phase_n.microW = 600.0F;
	phase_n.microVAR = 150.0F;
	phase_n.microVA = 650.0F;
	phase_n.Voltage_Angle = 5.0F;
	phase_n.Current_Angle = 15.0F;
	phase_n.PF = 0.98F;
	phase_n.microAmps_fundamental = 4.5F;
	phase_n.microVolts_fundamental = 55.0F;
	phase_n.Current_percentage_THD = 2.0F;
	phase_n.Voltage_percentage_THD = 1.5F;
	phase_n.PF_Angle = 10.0F;
	phase_n.Current_percentage_TDD = 1.0F;
	phase_n.Harmonic_Current = 0.2F;
	phase_n.Phase_Voltage_percentage_2nd_Harmonic = 0.5F;

	other.message_version = 1;
	other.timestamp = timestamp;
	other.Neutral_Imputed_microAmps = 5.5F;
	other.Load_Side_microVolts = 123.0F;

	// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

	response->timestamp = timestamp;
	response->phase_A = phase_a;
	response->has_phase_A = true;
	response->phase_B = phase_b;
	response->has_phase_B = true;
	response->phase_C = phase_c;
	response->has_phase_C = true;
	response->phase_N = phase_n;
	response->has_phase_N = true;
	response->other = other;
	response->has_other = true;
}

static void *gapi_instantaneous_thread(void *arg)
{
	struct mosquitto *mosq = (struct mosquitto *)arg;
	GeisaInstantaneousQuantities response;
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
