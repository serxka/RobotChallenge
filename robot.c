#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "robot.h"

// Our current program state, just a list of robots
static struct {
	struct robot robots[MAX_ROBOT_COUNT];
	size_t active;
	size_t selected;
} state;

int main(int argc, char *argv[]) {
	cmd_buf_t cmds;
	// Either read from stdin or a file
	if (argc < 2) {
		fprintf(stderr, "Press CTRL+D to exit input and run\n");
		cmds = parse(stdin);
	} else {
		FILE *fp = fopen(argv[1], "r");
		if (fp == NULL) {
			fprintf(stderr, "%s: Failed to open %s for reading\n",
			        argv[0], argv[1]);
			exit(EXIT_FAILURE);
		}
		cmds = parse(fp);
		fclose(fp);
	}
	// Run the commands
	exec(cmds);
	cmd_buf_free(&cmds);
}

void exec(cmd_buf_t cmds) {
	for (size_t i = 0; i < cmds.len; ++i) {
		command_t *cmd = cmds.data + i;
		struct robot *rb = state.robots + state.selected;

		// Skip command if robot not placed
		if (!rb->is_placed
		    && !(cmd->type == CMD_PLACE || cmd->type == CMD_SELECT))
			continue;

		switch (cmd->type) {
		case CMD_PLACE: {
			assert(state.active <= MAX_ROBOT_COUNT);
			struct robot *rb = state.robots + state.active;
			rb->x = cmd->args.place.x;
			rb->y = cmd->args.place.y;
			rb->heading = cmd->args.place.heading;
			rb->is_placed = true;

			// Increase the robot count
			state.selected = state.active;
			state.active += 1;
			break;
		}
		case CMD_MOVE:
			switch (rb->heading) {
			case DIR_NORTH:
				rb->y += rb->y == TABLE_HEIGHT ? 0 : 1;
				break;
			case DIR_EAST:
				rb->x += rb->x == TABLE_WIDTH ? 0 : 1;
				break;
			case DIR_SOUTH:
				rb->y -= rb->y == 0 ? 0 : 1;
				break;
			case DIR_WEST:
				rb->x -= rb->x == 0 ? 0 : 1;
				break;
			}
			break;
		case CMD_LEFT:
			if (rb->heading == DIR_NORTH)
				rb->heading = DIR_WEST;
			else
				rb->heading -= 1;
			break;
		case CMD_RIGHT:
			if (rb->heading >= DIR_WEST)
				rb->heading = DIR_NORTH;
			else
				rb->heading += 1;
			break;
		case CMD_REPORT:
			printf("Robot %lu of %lu: %u,%u,%s\n", state.selected + 1,
			       state.active, rb->x, rb->y,
			       e_heading_str[rb->heading]);
			break;
		case CMD_SELECT:
			assert(cmd->args.select <= MAX_ROBOT_COUNT && cmd->args.select != 0);
			if (cmd->args.select > state.active)
				break;
			if (state.robots[cmd->args.select - 1].is_placed == false)
				break;
			state.selected = cmd->args.select - 1;
			break;
		default:
			abort();
		}
	}
}

static void cmd_buf_grow(cmd_buf_t *buf) {
	size_t new_cap = buf->cap + 32;
	command_t *data =
	        (command_t *)realloc(buf->data, new_cap * sizeof(command_t));
	if (data == NULL)
		abort();
	buf->data = data;
	buf->cap = new_cap;
}

void cmd_buf_push(cmd_buf_t *buf, command_t cmd) {
	if (buf->len >= buf->cap)
		cmd_buf_grow(buf);
	buf->data[buf->len++] = cmd;
}

void cmd_buf_free(cmd_buf_t *buf) {
	if (buf->data)
		free(buf->data);
	*buf = (cmd_buf_t){0};
}
