#ifndef TEST_BUILD

#include <iostream>
#include <fstream>
#include <sstream>
#include <util\position.h>
#include <imaging\bmp-format.h>
#include <midi\midi.h>
#include <midi/primitives.h>
#include <iomanip>

void draw_rectangle(unsigned width, unsigned height, Position position, imaging::Color color, imaging::Bitmap& bitmap) {
	for (int x = position.x; x < width + position.x; x++)
	{
		for (int y = position.y; y < height + position.y; y++) {
			bitmap[Position(x, y)] = color;
		}
	}
}

std::vector<midi::NOTE> read_file(std::string filename) {
	char path[200];
	char path_char[1000];
	std::strcpy(path_char, filename.c_str());
	std::ifstream file(path_char, std::ios::binary);
	return midi::read_notes(file);
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

imaging::Color get_color_instrument(midi::NoteNumber note) {
	int nr = value(note) % 7;
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
	}
}

std::string padd_with_zeroes(int counter) {
	std::stringstream ss;
	ss << std::setfill('0') << std::setw(5) << counter;
	return ss.str();
}

int main(int argn, char** argv)
{
	std::vector<midi::NOTE> notes = read_file("C:\\Users\\Admin\\Desktop\\bmp\\mystery.mid");
	int duration = value((notes[notes.size() - 1].start + notes[notes.size() - 1].duration)) / 10;
	int highest_note;
	int lowest_note = 10000;
	highest_note_number(notes, highest_note, lowest_note);
	int height = ((highest_note - lowest_note) * 16) + 16;
	imaging::Bitmap bitmap(duration, height);
	for (int i = 0; i < notes.size(); i++)
	{
		Position position(value(notes[i].start) / 10, (highest_note - value(notes[i].note_number)) * 16);
		draw_rectangle(value(notes[i].duration) / 10, 16, position, get_color_instrument(notes[i].note_number), bitmap);
	}
	int width = 1500;
	int counter = 0;
	for (int i = 0; i + width <= duration; i+= 5)
	{
		imaging::save_as_bmp("D:\\Midi\\frame" + padd_with_zeroes(counter) + ".bmp", *bitmap.slice(i, 0, width, height));
		counter += 1;
	}
	imaging::save_as_bmp("C:\\Users\\Admin\\Desktop\\bmp\\bmp.bmp", bitmap);
}

#endif