/*
 * This file is part of bino, a 3D video player.
 *
 * Copyright (C) 2010-2011
 * Martin Lambers <marlam@marlam.de>
 * Alexey Osipov <lion-simba@pridelands.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <string>

#include "msg.h"
#include "s11n.h"

#include "controller.h"
#include "media_data.h"
#include "media_input.h"
#include "audio_output.h"
#include "video_output.h"


class player_init_data : public s11n
{
public:
    // Level of log messages
    msg::level_t log_level;
    // Input media objects
    std::vector<std::string> urls;
    // Selected video stream
    int video_stream;
    // Selected audio stream
    int audio_stream;
    // Benchmark mode?
    bool benchmark;
    // Make video fullscreen?
    bool fullscreen;
    // Center video on screen?
    bool center;
    // Manual input layout override
    bool stereo_layout_override;
    video_frame::stereo_layout_t stereo_layout;
    bool stereo_layout_swap;
    // Manual output mode override
    bool stereo_mode_override;
    parameters::stereo_mode_t stereo_mode;
    bool stereo_mode_swap;
    // Initial output parameters
    parameters params;

public:
    player_init_data();
    ~player_init_data();

    // Serialization
    void save(std::ostream &os) const;
    void load(std::istream &is);
};

class player
{
public:
    enum type
    {
        master, slave
    };

private:
    media_input *_media_input;
    std::vector<controller *> _controllers;
    audio_output *_audio_output;
    video_output *_video_output;
    parameters _params;
    bool _benchmark;

    bool _running;
    bool _first_frame;
    bool _need_frame;
    bool _drop_next_frame;
    bool _previous_frame_dropped;
    bool _in_pause;

    bool _quit_request;
    bool _pause_request;
    int64_t _seek_request;
    float _set_pos_request;

    video_frame _video_frame;

    size_t _required_audio_data_size;
    int64_t _pause_start;
    // Audio / video timing, relative to a synchronization point.
    // The master time is the audio time, or external time if there is no audio.
    // All times are in microseconds.
    int64_t _start_pos;                 // initial input position
    int64_t _current_pos;               // current input position
    int64_t _video_pos;                 // presentation time of current video frame
    int64_t _audio_pos;                 // presentation time of current audio data block
    int64_t _master_time_start;         // master time offset
    int64_t _master_time_current;       // current master time
    int64_t _master_time_pos;           // input position at master time start

    int _frames_shown;                   // frames shown since last _sync_point_time update
    int64_t _fps_mark_time;              // time when _frames_shown was reset to zero

    float normalize_pos(int64_t pos);   // transform a position into [0,1]

    void reset_playstate();

protected:
    virtual audio_output *create_audio_output();
    virtual video_output *create_video_output();

    void make_master();
    void step(bool *more_steps, int64_t *seek_to, bool *prep_frame, bool *drop_frame, bool *display_frame);
    bool run_step();

    media_input &get_media_input_nonconst()
    {
        return *_media_input;
    }

    void notify(const notification &note);
    void notify(enum notification::type t, bool p, bool c) { notify(notification(t, p, c)); }
    void notify(enum notification::type t, int p, int c) { notify(notification(t, p, c)); }
    void notify(enum notification::type t, float p, float c) { notify(notification(t, p, c)); }
    void notify(enum notification::type t, const std::string &p, const std::string &c) { notify(notification(t, p, c)); }

public:
    /* Constructor/destructor.
     * Only a single player instance can exist. The constructor throws an
     * exception if and only if it detects that this instance already exists. */
    player(type t = master);
    virtual ~player();

    /* Open a player. */
    virtual void open(const player_init_data &init_data);

    /* Get information about input and output parameters */
    const media_input &get_media_input() const
    {
        return *_media_input;
    }
    const parameters &get_parameters() const
    {
        return _params;
    }

    /* Run the player. It will take care of all interaction. This function
     * returns when the user quits the player. */
    virtual void run();

    /* Close the player and clean up. */
    virtual void close();

    /* Receive a command from a controller. */
    virtual void receive_cmd(const command &cmd);
};

#endif
