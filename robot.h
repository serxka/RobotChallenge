#ifndef _ROBOT_H_
#define _ROBOT_H_ 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_ROBOT_COUNT (16)
#define TABLE_WIDTH (4)
#define TABLE_HEIGHT (4)

#define LEN(x) (sizeof((x)) / sizeof((x)[0]))
#define ABS(x) ((x) < 0 ? -(x) : (x))

enum direction { DIR_NORTH = 0, DIR_EAST, DIR_SOUTH, DIR_WEST };

static const char *e_heading_str[] = {"NORTH", "EAST", "SOUTH", "WEST"};

// A structure holding possible instructions that can be run for the system
typedef struct {
	// An enum to contain the type of instruction we have
	enum cmd_type {
		CMD_PLACE,
		CMD_MOVE,
		CMD_LEFT,
		CMD_RIGHT,
		CMD_REPORT,
		CMD_SELECT,
		CMD_NONE,
	} type;
	// A union for any possible operands we need for the instruction
	union {
		// CMD_PLACE
		struct {
			uint32_t x, y;          // Our starting location
			enum direction heading; // Direction we are face
		} place;
		// CMD_SELECT
		size_t select; // We robot we want to select
	} args;
} command_t;

// Managed storage for a list of commands
typedef struct {
	command_t *data;
	size_t len;
	size_t cap;
} cmd_buf_t;

// This stores all the information we need about our robot
struct robot {
	uint32_t x, y;
	enum direction heading;
	bool is_placed;
};

cmd_buf_t parse(FILE *in);
void exec(cmd_buf_t cmds);

void cmd_buf_push(cmd_buf_t *buf, command_t cmd);
void cmd_buf_free(cmd_buf_t *buf);

#endif // _ROBOT_H_
