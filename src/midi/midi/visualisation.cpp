#include "visualisation.h"
#include <midi\midi.h>
#include <midi/primitives.h>
#include <regex>
#include <iomanip>
#include <fstream>
#include <sstream>

std::vector<midi::NOTE> read_file(std::string filename) {
	char path[200];
	char path_char[1000];
	std::strcpy(path_char, filename.c_str());
	std::ifstream file(path_char, std::ios::binary);
	return midi::read_notes(file);
}

int get_duration(std::vector<midi::NOTE> notes) {
	int duration = 0;
	for (midi::NOTE note : notes)
	{
		if (value(note.duration + note.start) > duration) {
			duration = value(note.duration + note.start);
		}
	}
	return duration;
}

imaging::Color get_color_instrument(midi::Instrument instrument) {
	int nr = value(instrument) % 10;
	switch (nr)
	{
	case 0:
		return imaging::colors::cyan();
	case 1:
		return imaging::colors::red();
	case 2:
		return imaging::colors::green();
	case 3:
		return imaging::colors::blue();
	case 4:
		return imaging::colors::yellow();
	case 5:
		return imaging::colors::magenta();
	case 6:
		return imaging::colors::orange();
	case 7:
		return imaging::colors::white();
	case 8:
		return imaging::Color(0.53, 0.06, 0.90);
	case 9:
		return imaging::Color(0.79, 0.17, 0.57);
	default:
		return imaging::Color(0.13, 0.54, 0.011);
	}
}

void highest_note_number(std::vector<midi::NOTE> notes, int& highest_note, int& lowest_note) {
	for (int i = 0; i < notes.size(); i++)
	{
		if (value(notes[i].note_number) > highest_note)
		{
			highest_note = value(notes[i].note_number);
		}
		if (value(notes[i].note_number) < lowest_note && value(notes[i].note_number) != 0)
		{
			lowest_note = value(notes[i].note_number);
		}
	}
}

std::string padd_with_zeroes(int counter) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(5) << counter;
	return ss.str();
}

void visualisation::draw_rectangle(unsigned width, unsigned height, Position position, imaging::Color color, imaging::Bitmap& bitmap) {
	for (int x = position.x; x < width + position.x; x++)
	{
		for (int y = position.y; y < height + position.y; y++) {
			bitmap[Position(x, y)] = color;
		}
	}
}

void visualisation::slice(imaging::Bitmap& bitmap, Visualizer& visualizer, int duration, int height) {
	int width = visualizer.frame_width;
	int counter = 0;
	for (int i = 0; i + width <= duration; i += visualizer.step)
	{
		imaging::save_as_bmp(frame_formatter(visualizer, counter), *bitmap.slice(i, 0, width, height));
		counter += 1;
	}
}

void visualisation::draw_bitmap(Visualizer& visualizer) {
	std::vector<midi::NOTE> notes = read_file(visualizer.filename);
	double scale = static_cast<double>(visualizer.horizontal_scale) / 100;
	int duration = (get_duration(notes) / 10) * scale;
	if (visualizer.frame_width == 0 || visualizer.frame_width > duration)
	{
		visualizer.frame_width = duration;
	}
	int highest_note;
	int lowest_note = 10000;
	highest_note_number(notes, highest_note, lowest_note);
	int height = ((highest_note - lowest_note) * visualizer.note_height) + visualizer.note_height;
	imaging::Bitmap bitmap(duration, height);
	for (int i = 0; i < notes.size(); i++)
	{
		Position position((value(notes[i].start) / 10) * scale, (highest_note - value(notes[i].note_number)) * visualizer.note_height);
		visualisation::draw_rectangle((value(notes[i].duration) / 10) * scale, visualizer.note_height, position, get_color_instrument(notes[i].instrument), bitmap);
	}
	visualisation::slice(bitmap, visualizer, duration, height);
}

std::string visualisation::frame_formatter(Visualizer& visualizer, int counter)
{
	std::regex d("%d");
	return std::regex_replace(visualizer.pattern, d, padd_with_zeroes(counter));
}