/*
   Vimpc
   Copyright (C) 2013 Nathan Sweetman

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   command.cpp - tests for command mode code
   */

#include <cppunit/extensions/HelperMacros.h>

#include "algorithm.hpp"
#include "test.hpp"
#include "mode/command.hpp"
#include "window/debug.hpp"
#include "window/error.hpp"

class CommandTester : public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(CommandTester);
   CPPUNIT_TEST(CommandSplit);
   CPPUNIT_TEST(Execution);

   CPPUNIT_TEST(SetCommand);
   CPPUNIT_TEST(TabCommands);
   CPPUNIT_TEST(SetActiveWindowCommands);
   CPPUNIT_TEST(StateCommands);
   CPPUNIT_TEST_SUITE_END();

public:
   CommandTester() :
      settings_(Main::Settings::Instance()),
      commandMode_(*Main::Tester::Instance().Command),
      screen_(*Main::Tester::Instance().Screen),
      client_(*Main::Tester::Instance().Client) 
      { }

public:
   void setUp();
   void tearDown();

protected:
   void CommandSplit();
   void Execution();

   void SetCommand();
   void TabCommands();
   void SetActiveWindowCommands();
   void StateCommands();

private:
   Main::Settings & settings_;
   Ui::Command    & commandMode_;
   Ui::Screen     & screen_;
   Mpc::Client    & client_;
   int32_t          window_;
};

void CommandTester::setUp()
{
   Ui::ErrorWindow::Instance().ClearError();
   window_ = screen_.GetActiveWindow();
}

void CommandTester::tearDown()
{
   screen_.SetActiveAndVisible(window_);
   Ui::ErrorWindow::Instance().ClearError();
}

void CommandTester::CommandSplit()
{
   std::string range, command, arguments;
   commandMode_.SplitCommand("echo", range, command, arguments);
   CPPUNIT_ASSERT((range == "") && (command == "echo") && (arguments == ""));

   commandMode_.SplitCommand("echo test", range, command, arguments);
   CPPUNIT_ASSERT((range == "") && (command == "echo") && (arguments == "test"));

   commandMode_.SplitCommand("1echo testing", range, command, arguments);
   CPPUNIT_ASSERT((range == "1") && (command == "echo") && (arguments == "testing"));

   commandMode_.SplitCommand("1,2echo testing", range, command, arguments);
   CPPUNIT_ASSERT((range == "1,2") && (command == "echo") && (arguments == "testing"));

   commandMode_.SplitCommand("1,2echo testing a longer argument", range, command, arguments);
   CPPUNIT_ASSERT((range == "1,2") && (command == "echo") && (arguments == "testing a longer argument"));

   std::vector<std::string> args = commandMode_.SplitArguments(arguments);
   CPPUNIT_ASSERT(args.size() == 4);
   CPPUNIT_ASSERT((args[0] == "testing") && (args[1] == "a") && (args[2] == "longer") && (args[3] == "argument"));
}

void CommandTester::Execution()
{
   // Ensure that the error command is run and that an error is set
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("error 1");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();

   // Test aliasing of commands to another name
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("alias thisisnotarealcommand sleep 0");
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("unalias thisisnotarealcommand");
   commandMode_.ExecuteCommand("thisisnotarealcommand");
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();
}


void CommandTester::SetCommand()
{
   // Test that calling the set command changes a setting
   std::string window = settings_.Get(Setting::Window);
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " playlist");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "playlist");
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " test");
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == "test");
   commandMode_.ExecuteCommand("set " + settings_.Name(Setting::Window) + " " + window);
   CPPUNIT_ASSERT(settings_.Get(Setting::Window) == window);
}

void CommandTester::TabCommands()
{
   char Buffer[128];

   // Ensure that we change to the first tab
   commandMode_.ExecuteCommand("tabfirst");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   // Ensure that we change to the last tab
   commandMode_.ExecuteCommand("tablast");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == (screen_.VisibleWindows() - 1));

   // Ensure that we can move the current tab to position 0 and back
   screen_.SetActiveAndVisible(window_);
   int32_t Index = screen_.GetActiveWindowIndex();
   commandMode_.ExecuteCommand("tabmove 0");
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);

   snprintf(Buffer, 128, "%d", Index);
   commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
   CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

   // Try and rename a tab and ensure the name changes
   std::string name = screen_.GetNameFromWindow(screen_.GetActiveWindow());
   commandMode_.ExecuteCommand("tabrename aridiculoustestname");
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == "aridiculoustestname");
   commandMode_.ExecuteCommand("tabrename " + name);
   CPPUNIT_ASSERT(screen_.GetNameFromWindow(screen_.GetActiveWindow()) == name);

   if (window_ < (int32_t) Ui::Screen::MainWindowCount)
   {
      screen_.SetActiveAndVisible(window_);

      // Test closing the current tab
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      commandMode_.ExecuteCommand("tabclose");
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == false);
      screen_.SetActiveAndVisible(window_);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window_);

      // Move the current tab back to the correct position
      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);

      // Test hiding/closing a tab by name
      screen_.SetActiveWindow(0);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      commandMode_.ExecuteCommand("tabhide " + name);
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == false);

      // If we didn't close the active tab we should not
      // change which window is focused
      if (Index != 0)
      {
         CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == 0);
      }

      screen_.SetActiveAndVisible(window_);
      CPPUNIT_ASSERT(screen_.GetActiveWindow() == window_);

      // Move the tab back to where it started
      snprintf(Buffer, 128, "%d", Index);
      commandMode_.ExecuteCommand("tabmove " + std::string(Buffer));
      CPPUNIT_ASSERT(screen_.IsVisible(window_) == true);
      CPPUNIT_ASSERT(screen_.GetActiveWindowIndex() == Index);
   }
}

void CommandTester::SetActiveWindowCommands()
{
   // Make sure that the correct windows are opened
   commandMode_.ExecuteCommand("browse");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Browse);
   commandMode_.ExecuteCommand("console");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Console);
   commandMode_.ExecuteCommand("help");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Help);
   commandMode_.ExecuteCommand("library");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Library);
   commandMode_.ExecuteCommand("directory");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Directory);
   commandMode_.ExecuteCommand("playlist");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Playlist);
   commandMode_.ExecuteCommand("outputs");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Outputs);
   commandMode_.ExecuteCommand("lists");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::Lists);
   commandMode_.ExecuteCommand("windowselect");
   CPPUNIT_ASSERT((Ui::Screen::MainWindow) screen_.GetActiveWindow() == Ui::Screen::WindowSelect);
}

void CommandTester::StateCommands()
{
   bool    Random    = client_.Random();
   bool    Single    = client_.Single();
   bool    Consume   = client_.Consume();
   bool    Repeat    = client_.Repeat();
   int32_t Crossfade = client_.Crossfade();
   int32_t Volume    = client_.Volume();
   std::string State = client_.CurrentState();

   // Test that all the states are toggled
   commandMode_.ExecuteCommand("random");
   CPPUNIT_ASSERT(Random != client_.Random());
   commandMode_.ExecuteCommand("repeat");
   CPPUNIT_ASSERT(Repeat != client_.Repeat());
   commandMode_.ExecuteCommand("single");
   CPPUNIT_ASSERT(Single != client_.Single());
   commandMode_.ExecuteCommand("consume");
   CPPUNIT_ASSERT(Consume != client_.Consume());

   // Test that all the states are turned on
   commandMode_.ExecuteCommand("random on");
   CPPUNIT_ASSERT(client_.Random() == true);
   commandMode_.ExecuteCommand("repeat on");
   CPPUNIT_ASSERT(client_.Repeat() == true);
   commandMode_.ExecuteCommand("single on");
   CPPUNIT_ASSERT(client_.Single() == true);
   commandMode_.ExecuteCommand("consume on");
   CPPUNIT_ASSERT(client_.Consume() == true);

   // Test that all the states are turned off
   commandMode_.ExecuteCommand("random off");
   CPPUNIT_ASSERT(client_.Random() == false);
   commandMode_.ExecuteCommand("repeat off");
   CPPUNIT_ASSERT(client_.Repeat() == false);
   commandMode_.ExecuteCommand("single off");
   CPPUNIT_ASSERT(client_.Single() == false);
   commandMode_.ExecuteCommand("consume off");
   CPPUNIT_ASSERT(client_.Consume() == false);

   // Restore their original values
   client_.SetRandom((Random == true));
   client_.SetRepeat((Repeat == true));
   client_.SetSingle((Single == true));
   client_.SetConsume((Consume == true));

   // If we are currently playing, pause before we go
   // messing about with the volume
   if (Algorithm::iequals(State, "playing") == true)
   {
      client_.Pause(); 
   }

   // Try min, max, mid and invalid volumes
   Ui::ErrorWindow::Instance().ClearError();
   commandMode_.ExecuteCommand("volume 0");
   CPPUNIT_ASSERT(client_.Volume() == 0);
   commandMode_.ExecuteCommand("volume 100");
   CPPUNIT_ASSERT(client_.Volume() == 100);
   commandMode_.ExecuteCommand("volume 50");
   CPPUNIT_ASSERT(client_.Volume() == 50);
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == false);
   commandMode_.ExecuteCommand("volume 500");
   CPPUNIT_ASSERT(client_.Volume() == 50);
   CPPUNIT_ASSERT(Ui::ErrorWindow::Instance().HasError() == true);
   Ui::ErrorWindow::Instance().ClearError();

   client_.SetVolume(Volume);

   // Set mpd to whatever state it was in before
   if (Algorithm::iequals(State, "playing") == true)
   {
      client_.Pause(); 
   }
}

CPPUNIT_TEST_SUITE_REGISTRATION(CommandTester);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CommandTester, "command");