#ifndef TEST_BUILD

#include <midi/visualisation.h>
#include <shell\command-line-parser.h>

int main(int argn, char** argv)
{
	shell::CommandLineParser parser;
	visualisation::Visualizer visualizer;
	bool fileFound = false;
	bool isValue = false;
	for (int i = 1; i < argn; i++)
	{
		std::string command(argv[i]);
		if (command == "-w")
		{
			parser.add_argument(command, &visualizer.frame_width);
			isValue = true;
		}
		else if (command == "-d")
		{
			parser.add_argument(command, &visualizer.step);
			isValue = true;
		}
		else if (command == "-h")
		{
			parser.add_argument(command, &visualizer.note_height);
			isValue = true;
		}
		else if (command == "-s")
		{
			parser.add_argument(command, &visualizer.horizontal_scale);
			isValue = true;
		}
		else if (!fileFound && !isValue)
		{
			fileFound = true;
			assert(command.find(".mid") != std::string::npos);
			visualizer.filename = command;
		}
		else if (fileFound && !isValue) {
			assert(command.find("%d") != std::string::npos);
			visualizer.pattern = command;
		} 
		else if (isValue)
		{
			isValue = false;
		}
	}
	parser.process(argn, argv);
	visualisation::draw_bitmap(visualizer);
}

#endif