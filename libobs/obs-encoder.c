/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs.h"
#include "obs-internal.h"

static inline struct obs_encoder_info *get_encoder_info(const char *id)
{
	for (size_t i = 0; i < obs->encoder_types.num; i++) {
		struct obs_encoder_info *info = obs->encoder_types.array+i;

		if (strcmp(info->id, id) == 0)
			return info;
	}

	return NULL;
}

const char *obs_encoder_getdisplayname(const char *id, const char *locale)
{
	struct obs_encoder_info *ei = get_encoder_info(id);
	if (!ei)
		return NULL;

	return ei->getname(locale);
}

obs_encoder_t obs_encoder_create(const char *id, const char *name,
		obs_data_t settings)
{
	struct obs_encoder *encoder;
	struct obs_encoder_info *ei = get_encoder_info(id);

	if (!ei)
		return NULL;

	encoder = bzalloc(sizeof(struct obs_encoder));
	encoder->info = *ei;

	if (pthread_mutex_init(&encoder->data_callbacks_mutex, NULL) != 0) {
		bfree(encoder);
		return NULL;
	}

	encoder->settings = obs_data_newref(settings);
	encoder->data     = ei->create(encoder->settings, encoder);

	if (!encoder->data) {
		pthread_mutex_destroy(&encoder->data_callbacks_mutex);
		obs_data_release(encoder->settings);
		bfree(encoder);
		return NULL;
	}

	pthread_mutex_lock(&obs->data.encoders_mutex);
	da_push_back(obs->data.encoders, &encoder);
	pthread_mutex_unlock(&obs->data.encoders_mutex);
	return encoder;
}

void obs_encoder_destroy(obs_encoder_t encoder)
{
	if (encoder) {
		pthread_mutex_lock(&obs->data.encoders_mutex);
		da_erase_item(obs->data.encoders, &encoder);
		pthread_mutex_unlock(&obs->data.encoders_mutex);

		encoder->info.destroy(encoder->data);
		obs_data_release(encoder->settings);
		bfree(encoder);
	}
}

obs_properties_t obs_encoder_properties(const char *id, const char *locale)
{
	const struct obs_encoder_info *ei = get_encoder_info(id);
	if (ei && ei->properties)
		return ei->properties(locale);
	return NULL;
}

void obs_encoder_update(obs_encoder_t encoder, obs_data_t settings)
{
	obs_data_replace(&encoder->settings, settings);
	encoder->info.update(encoder->data, encoder->settings);
}

bool obs_encoder_reset(obs_encoder_t encoder)
{
	return encoder->info.reset(encoder->data);
}

bool obs_encoder_encode(obs_encoder_t encoder, void *frames, size_t size)
{
	/* TODO */
	//encoder->info.encode(encoder->data, frames, size, packets);
	return false;
}

int obs_encoder_getheader(obs_encoder_t encoder,
		struct encoder_packet **packets)
{
	return encoder->info.getheader(encoder, packets);
}

bool obs_encoder_setbitrate(obs_encoder_t encoder, uint32_t bitrate,
		uint32_t buffersize)
{
	if (encoder->info.setbitrate)
		return encoder->info.setbitrate(encoder->data, bitrate,
				buffersize);
	return false;
}

bool obs_encoder_request_keyframe(obs_encoder_t encoder)
{
	if (encoder->info.request_keyframe)
		return encoder->info.request_keyframe(encoder->data);
	return false;
}

obs_data_t obs_encoder_get_settings(obs_encoder_t encoder)
{
	obs_data_addref(encoder->settings);
	return encoder->settings;
}
