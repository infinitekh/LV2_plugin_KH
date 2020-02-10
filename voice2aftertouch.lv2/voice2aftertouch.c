#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#    include <stdbool.h>
#endif

#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
//#include "lv2/lv2plug.in/ns/lv2core/lv2_util.h"


#define VOICE_AFTERTOUCH_URI "https://github.com/infinitekh/LV2_plugin_KH#voice2aftertouch"

enum {
	MIDI_IN  = 0,
	MIDI_OUT = 1,
	AUDIO_IN = 2,
	MIDI_PORT = 3,

};

typedef struct {
	// Features
	LV2_URID_Map*  map;
	LV2_Log_Logger logger;

	// Ports
	const LV2_Atom_Sequence* midi_in;
	LV2_Atom_Sequence*       midi_out;
	float*                  midi_port;
	float*                 audio_in;

	// URIs
} Voice2aftertouch;

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Voice2aftertouch* self = (Voice2aftertouch*)instance;
	switch (port) {
	case MIDI_IN:
		self->midi_in = (const LV2_Atom_Sequence*)data;
		break;
	case MIDI_OUT:
		self->midi_out = (LV2_Atom_Sequence*)data;
		break;
	case AUDIO_IN:
		self->audio_in = (float *)data;
		break;
	case MIDI_PORT:
		self->midi_port = (float *)data;
		break;
	default:
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
	// Allocate and initialise instance structure.
	Voice2aftertouch* self = (Voice2aftertouch*)calloc(1, sizeof(Voice2aftertouch));
	if (!self) {
		return NULL;
	}


	return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Voice2aftertouch*     self = (Voice2aftertouch*)instance;

	// Struct for a 3 byte MIDI event, used for writing notes
	typedef struct {
		LV2_Atom_Event event;
		uint8_t        msg[3];
	} MIDINoteEvent;

	// Initially self->midi_out contains a Chunk with size set to capacity

	// Get the capacity
	const uint32_t out_capacity = self->midi_out->atom.size;

	int midi_port =(int)( *(self->midi_port));

	// Write an empty Sequence header to the output
	lv2_atom_sequence_clear(self->midi_out);
	self->midi_out->atom.type = self->midi_in->atom.type;

	const float* input = self->audio_in;

	double sqsum=0;
	for (uint32_t pos = 0; pos < sample_count; pos++) {
		  sqsum = input[pos] * input[pos];
	}
	double msr = sqrt(sqsum/sample_count);

	int db=20.0*log(msr/0.0002);
	if (db >127) db=127;
	else if (db < 0) db = 0;


	// Read incoming events
	LV2_ATOM_SEQUENCE_FOREACH(self->midi_in, ev) {
		if (ev->body.type ) {
			const uint8_t* const msg = (const uint8_t*)(ev + 1);
			switch (lv2_midi_message_type(msg)) {
			case LV2_MIDI_MSG_NOTE_ON:
			case LV2_MIDI_MSG_NOTE_OFF:
				// Forward note to output
				lv2_atom_sequence_append_event(
					self->midi_out, out_capacity, ev);

				break;
			default:
				// Forward all other MIDI events directly
				lv2_atom_sequence_append_event(
					self->midi_out, out_capacity, ev);
				break;
			}
		}
	}
	MIDINoteEvent channel_pressure;
	channel_pressure.msg[0] = LV2_MIDI_MSG_CHANNEL_PRESSURE;
	channel_pressure.msg[1] = midi_port&0xF;
	channel_pressure.msg[2] = db;

	lv2_atom_sequence_append_event(self->midi_out, 
			out_capacity,
			&channel_pressure.event);
	

}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
VOICE_AFTERTOUCH_URI,
	instantiate,
	connect_port,
	NULL,  // activate,
	run,
	NULL,  // deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
