#ifndef SETTINGS_H
#define SETTINGS_H

const boolean DEBUG = false;

const boolean NOTIFY_UPCOMING_DEEP_SLEEP = true;

const boolean NOTIFY_DEEP_SLEEP = true;

const boolean SILENTLY_NOTIFY_PROJECT_STARTUP = true;

const boolean SILENTLY_NOTIFY_DOORBELL_RINGING = false;

const boolean SILENTLY_NOTIFY_UPCOMING_DEEP_SLEEP = false;

const boolean SILENTLY_NOTIFY_DEEP_SLEEP = false;

const unsigned int RUNTIME_BEFORE_UPCOMING_DEEP_SLEEP_NOTIFICATION_IN_MILLISECONDS = 3300000; // 55 minutes

const unsigned int RUNTIME_BEFORE_DEEP_SLEEP_IN_MILLISECONDS = 3600000; // 1 hour

/**
  https://core.telegram.org/bots/faq#my-bot-is-hitting-limits-how-do-i-avoid-this

  When sending messages inside a particular chat, avoid sending more than one message per second.
  Also note that your bot will not be able to send more than 20 messages per minute to the same group.

  Failsafe lock: 1 h = 60 m / 20 messages = 3 m = 3 * 60 s = 180 s = 180000 ms

  Sane delay: Assuming that it's very unlikey to get those amounts of doorbell rings
  I think it's safe to fall back to a 1 minute message lock.
*/
const unsigned int NOTIFICATION_LOCK_IN_MILLISECONDS = 60000; // one minute

const unsigned int NOTIFICATION_RETRIES = 3;

// Sample window width (50 ms = 20Hz)
const unsigned int SAMPLE_WINDOW_WIDTH_IN_MILLISECONDS = 50;

// https://hester.mtholyoke.edu/idesign/SensorAmp.html
const float SOUND_THRESHOLD = 973;

// https://apps.timwhitlock.info/emoji/tables/unicode
const char PROJECT_STARTUP_MESSAGE[] = "\xF0\x9F\x91\x82"; // `ear` emoji

const char DOORBELL_RINGING_MESSAGE[] = "Knock Knock";

const char UPCOMING_DEEP_SLEEP_MESSAGE[] = "\xF0\x9F\x98\xA9"; // `sleeping face` emoji

const char DEEP_SLEEP_MESSAGE[] = "\xF0\x9F\x92\xA4"; // `sleeping symbol` emoji

#endif
