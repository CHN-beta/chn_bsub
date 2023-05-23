# include <map>
# include <ftxui/component/component.hpp>
# include <ftxui/component/screen_interactive.hpp>
# include <boost/process.hpp>
# include <fmt/format.h>

using namespace fmt::literals;

int main()
{
	// 增加蓝色的标题栏
	auto wrap = [](std::string name, ftxui::Component component)
	{
		return ftxui::Renderer(component, [name, component]
		{
			return ftxui::vbox
			({
				ftxui::text(name) | ftxui::bgcolor(ftxui::Color::Blue),
				ftxui::separator(),
				component->Render() | ftxui::xflex,
				ftxui::separator()
			});
		}) | ftxui::xflex;
	};

	// 各种显示在界面上的选项
	int vasp_version_selected = 0;
	std::vector<std::string> vasp_version_entries =
	{
		"640", "640_fixc", "640_optcell_vtst_wannier90", "640_shmem", "640_vtst",
		"631_shmem"
	};
	int vasp_variant_selected = 0;
	std::vector<std::string> vasp_variant_entries = {"std", "gam", "ncl"};
	int queue_selected = 0;
	std::vector<std::string> queue_entries =
	{
		"normal_1day", "normal_1week", "normal",
		"normal_1day_new", "ocean_530_1day", "ocean6226R_1day"
	};
	std::string ncores = "";

	// 当 ncores 为空字符串时，使用默认的核数
	std::map<std::string, std::size_t> max_cores =
	{
		{"normal_1day", 28}, {"normal_1week", 28}, {"normal", 20},
		{"normal_1day_new", 24}, {"ocean_530_1day", 24}, {"ocean6226R_1day", 32}
	};

	// 最终提交任务的命令
	std::string bsub = "";

	// 界面上的按钮对应的行为
	std::string user_command = "";

	// 构建界面(不带边框)
	// 因为需要增加下划线，先构建 input
	auto screen = ftxui::ScreenInteractive::Fullscreen();
	auto ncores_input = ftxui::Input(&ncores, "(leave blank to use all cores)");
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
