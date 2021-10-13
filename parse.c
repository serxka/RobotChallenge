#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "robot.h"

static int64_t _getline(char **buf, size_t *len, FILE *stream);

static command_t parse_place(const char *line);
static command_t parse_select(const char *line);
static command_t parse_unknown(const char *line);

// This struct contains a list of rules for how we should parse certain
// commands and a callback to parse any additional parameters needed
static const struct rule {
	const char *verb;
	enum cmd_type type;
	command_t (*adj_cb)(const char *);
} parse_rules[] = {
        {"PLACE ", CMD_PLACE, parse_place},
        {"MOVE", CMD_MOVE, NULL},
        {"LEFT", CMD_LEFT, NULL},
        {"RIGHT", CMD_RIGHT, NULL},
        {"REPORT", CMD_REPORT, NULL},
        {"ROBOT ", CMD_SELECT, parse_select},
        // This rule will be run if the command verb is unknown, must be last
        {NULL, CMD_NONE, parse_unknown}};

cmd_buf_t parse(FILE *in) {
	char *line = NULL;
	size_t line_len = 0;
	cmd_buf_t cmds = {0};

	// Continue to loop until we return
	for (;;) {
		// Try and read the current line, if < -1, either error or EOF
		int64_t chars;
		if ((chars = _getline(&line, &line_len, in)) < 0) {
			if (line)
				free(line);
			return cmds;
		}

		// The line was empty, continue on
		if (*line == '\0')
			continue;

		bool found = false;
		// Loop through the rules and run strcmp on the verb
		for (size_t i = 0; i < LEN(parse_rules) - 1; ++i) {
			const struct rule *rule = parse_rules + i;
			if (!strncmp(rule->verb, line, strlen(rule->verb))) {
				if (rule->adj_cb != NULL)
					cmd_buf_push(&cmds, rule->adj_cb(line));
				else
					cmd_buf_push(
					        &cmds,
					        (command_t){
					                .type = rule->type});
				found = true;
				break;
			}
		}
		// If we didn't match a verb, run our fallback parse
		if (!found) {
			const struct rule *rule =
			        parse_rules + LEN(parse_rules) - 1;
			command_t cmd = rule->adj_cb(line);
			if (cmd.type != CMD_NONE)
				cmd_buf_push(&cmds, cmd);
		}
	}
}

// This is a typedef so we can print have some helpful data for when an
// error occurs, in this case we just have a typedef for a little message
typedef struct {
	const char *msg;
	const char *line;
} context_t;
static void print_context(context_t ctx) {
	fprintf(stderr, "failed parsing: %s, line: %s\n", ctx.msg, ctx.line);
}

static void skip_whitespace(const char **line) {
	const char *l = *line;
	while (*l == ' ' || *l == '\t' || *l == '\r' || *l == '\n')
		++l;
	*line = l;
}

static void expect_char(char c, const char **line, context_t ctx) {
	if (**line != c) {
		print_context(ctx);
		exit(EXIT_FAILURE);
	}
	*line += 1;
}

static uint64_t parse_number(const char **line, context_t ctx) {
	char *end;
	int64_t n = strtoll(*line, &end, 10);
	if (*line == end) { // We failed to parse a number
		print_context(ctx);
		exit(EXIT_FAILURE);
	}
	*line = end;
	return (uint64_t)ABS(n);
}

static enum direction parse_direction(const char **line, context_t ctx) {
	enum direction heading;
	bool matched = false;
	for (size_t i = 0; i < LEN(e_heading_str); ++i) {
		if (!strcmp(e_heading_str[i], *line)) {
			heading = i;
			matched = true;
			break;
		}
	}
	if (!matched) { // If we failed to match anything
		print_context(ctx);
		exit(EXIT_FAILURE);
	}
	*line += strlen(e_heading_str[heading]);
	return heading;
}

static command_t parse_place(const char *line) {
	const char *orginal = line;
	line += 6; // Skip the 'PLACE ' part of the line

	// Parse our numbers
	uint32_t x = parse_number(&line,
	                          (context_t){"X component of PLACE", orginal});
	expect_char(',', &line, (context_t){"expected X comma", orginal});
	uint32_t y = parse_number(&line,
	                          (context_t){"Y component of PLACE", orginal});
	expect_char(',', &line, (context_t){"expected Y comma", orginal});
	// Parse our direction
	enum direction heading = parse_direction(
	        &line, (context_t){"direction component of PLACE", orginal});

	return (command_t){
	        .type = CMD_PLACE,
	        .args = {.place = {.x = x, .y = y, .heading = heading}}};
}

static command_t parse_select(const char *line) {
	const char *orginal = line;
	line += 6; // Skip the 'ROBOT ' part of the line

	// Parse an index
	size_t i = (size_t)parse_number(
	        &line, (context_t){"expected index of ROBOT", orginal});

	return (command_t){.type = CMD_SELECT,
	                   .args = {
	                           .select = i,
	                   }};
}

static command_t parse_unknown(const char *line) {
	const char *orginal = line;
	// Check to see if it's only whitespace
	skip_whitespace(&line);
	if (*line) {
		fprintf(stderr,
		        "cannot parse line, unexpected characters: %s\n",
		        orginal);
		exit(EXIT_FAILURE);
	}
	return (command_t){.type = CMD_NONE};
}

// The glibc function was a bit of a headache, made this instead
static int64_t _getline(char **buf, size_t *len, FILE *stream) {
	assert(buf != NULL && len != NULL);

	if (*buf == NULL || *len == 0) {
		*len = 128;
		*buf = (char *)malloc(*len * sizeof(char));
		if (buf == NULL)
			return -1;
	}

	char *cursor = *buf;
	for (;;) {
		// Read a character
		int c = getc(stream);

		// If there was an error, or if we hit EOF at the start, return
		if (ferror(stream) || (cursor == *buf && c == EOF))
			return -1;

		// Break and return if EOF, or a newline character
		if (c == EOF || c == '\n' || c == '\r')
			break;

		// Check if we need to resize
		if ((*buf + *len - cursor) <= 1) {
			// Determine the new length
			size_t new_len = *len * 2;
			assert(new_len > *len);

			char *new_buf = (char *)realloc(*buf, new_len);
			if (new_buf == NULL)
				return -1;
			// Get the offset and set the new cursor
			cursor = new_buf + (cursor - *buf);
			*buf = new_buf;
			*len = new_len;
		}

		*cursor++ = (char)c;
	}

	*cursor = '\0';
	return (int64_t)(cursor - *buf);
}
