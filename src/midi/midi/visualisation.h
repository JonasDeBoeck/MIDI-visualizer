#ifndef VISUALISATION_H
#define VISUALISATION_H

#include <imaging/bmp-format.h>
#include <vector>

namespace visualisation {
	struct Visualizer {
		unsigned frame_width, step, note_height, horizontal_scale;
		std::string filename, pattern;
		Visualizer() : frame_width(0), step(1), note_height(16), horizontal_scale(100), filename(""), pattern("") {};
	};

	void draw_rectangle(unsigned width, unsigned height, Position position, imaging::Color color, imaging::Bitmap& bitmap);
	void draw_bitmap(Visualizer& visualizer);
	void slice(imaging::Bitmap& bitmap, Visualizer& visualizer, int duration, int height);
	std::string frame_formatter(Visualizer& visualizer, int counter);
}

#endif