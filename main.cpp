# include <map>
# include <ftxui/component/component.hpp>
# include <ftxui/component/screen_interactive.hpp>
# include <boost/process.hpp>
# include <fmt/format.h>

using namespace fmt::literals;

// 为组件增加标题栏
ftxui::Component component_with_title(std::string title, ftxui::Component component)
{
	return ftxui::Renderer(component, [title, component]
	{
		return ftxui::vbox
		({
			ftxui::text(title) | ftxui::bgcolor(ftxui::Color::Blue),
			component->Render(),
			ftxui::separator()
		});
	});
}

// UI:
// |---------------------------------------------|
// | select vasp version:                        |
// | > 640  | > (default)               | > std  |
// |   631  |   fixc 				    |   gam  |
// |        |   optcell_vtst_wannier90  |   ncl  |
// |        |   shmem 				    |        |
// |        |   vtst  				    |        |
// |---------------------------------------------|
// | select queue:							     |
// | > normal_1day                               |
// |   normal_1week                              |
// |   normal                                    |
// |   normal_1day_new                           |
// |   ocean_530_1day                            |
// |   ocean6226R_1day                           |
// |---------------------------------------------|
// | input cores you want to use:                |
// | (leave blank to use all cores)              |
// |---------------------------------------------|
// | job name:                                   |
// | my-great-job                                |
// |---------------------------------------------|
// | ------------ --------                       |
// | | continue | | quit |     			         |
// | ------------ --------       		         |
// |---------------------------------------------|
int main()
{
	// 需要绑定到界面上的变量
	struct
	{
		std::array<int, 3> vasp_version_selected = {0, 0, 0};
		std::vector<std::string> vasp_version_entries_level1 = {"640", "631"};
		std::map<std::string, std::vector<std::string>> vasp_version_entries_level2 = 
		{
			{"640", {"(default)", "fixc", "optcell_vtst_wannier90", "shmem", "vtst"}},
			{"631", {"shmem"}}
		};
		std::vector<std::string> vasp_version_entries_level3 = {"std", "gam", "ncl"};

		int queue_selected = 0;
		std::vector<std::string> queue_entries =
		{
			"normal_1day", "normal_1week", "normal",
			"normal_1day_new", "ocean_530_1day", "ocean6226R_1day"
		};
		std::map<std::string, std::size_t> max_cores =
		{
			{"normal_1day", 28}, {"normal_1week", 28}, {"normal", 20},
			{"normal_1day_new", 24}, {"ocean_530_1day", 24}, {"ocean6226R_1day", 32}
		};
		std::string ncores = "";
		std::string bsub = "";
		std::string user_command = "";
	} state;

	// 构建界面, 需要至少 25 行 49 列
	auto screen = ftxui::ScreenInteractive::FixedSize(30, 30);
	auto request_interface = [&]
	{
		auto ncores_input = ftxui::Input(&ncores, "(leave blank to use all cores)");
	}();
	
	auto request_interface = ftxui::Container::Vertical
	({
		wrap("Select vasp version:",
			ftxui::Menu(&vasp_version_entries, &vasp_version_selected)),
		wrap("Select vasp variant:",
			ftxui::Toggle(&vasp_variant_entries, &vasp_variant_selected)),
		wrap("Select queue:",
			ftxui::Menu(&queue_entries, &queue_selected)),
		wrap("Input cores you want to use:",
			ftxui::Renderer(ncores_input, [&]{return ftxui::underlined(ncores_input->Render());})),
			ftxui::Container::Horizontal
			({
				ftxui::Button("Continue",
					[&]{user_command = "continue"; screen.ExitLoopClosure()();}),
				ftxui::Button("Quit",
					[&]{user_command = "quit"; screen.ExitLoopClosure()();})
			})
	});
	auto bsub_input = ftxui::Input(&bsub, "");
	auto final_interface = ftxui::Container::Vertical
	({
		wrap("Submit using:",
			ftxui::Renderer(bsub_input, [&]{return ftxui::underlined(bsub_input->Render());})),
		ftxui::Container::Horizontal
		({
			ftxui::Button("Submit",
				[&]{user_command = "submit"; screen.ExitLoopClosure()();}),
			ftxui::Button("Quit",
				[&]{user_command = "quit"; screen.ExitLoopClosure()();}),
				ftxui::Button("Back",
					[&]{user_command = "back"; screen.ExitLoopClosure()();})
		})
	});

	// 实际投递任务
	auto submit = [](std::string bsub)
	{
		auto process = boost::process::child
		(
			boost::process::search_path("sh"), "-c", bsub,
			boost::process::std_in.close(),
			boost::process::std_out > stdout,
			boost::process::std_err > stderr
		);
		process.wait();
	};

	// 进入事件循环
	while (true)
	{
		screen.Loop(request_interface);
		if (user_command == "quit")
			return EXIT_FAILURE;
		else if (user_command != "continue")
			throw std::runtime_error("user_command is not recognized");
		bsub = fmt::format
		(
			R"(bsub -J "my-great-job" -q {} -n {} -R "span[hosts=1]" -o "output.txt" chn_vasp.sh {}_{})",
			queue_entries[queue_selected],
			ncores.empty() ? max_cores[queue_entries[queue_selected]] : std::stoi(ncores),
			vasp_version_entries[vasp_version_selected],
			vasp_variant_entries[vasp_variant_selected]
		);
		screen.Loop(final_interface);
		if (user_command == "quit")
			return EXIT_FAILURE;
		else if (user_command == "back")
			continue;
		else if (user_command != "submit")
			throw std::runtime_error("user_command is not recognized");
		submit(bsub);
		break;
	}
}
