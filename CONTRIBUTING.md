# Contributing

Thank you for helping improve the LR 10K Control Software. It is used in teaching labs, so changes must keep the machine safe to run and the results correct.

## Before you start

- **Open an issue first** for anything bigger than a small fix, so we can agree on the approach before you spend time on it. Not everyone working on the software has access to an LR 10K.
- Read [ARCHITECTURE.md](ARCHITECTURE.md) (what each file does) and the developer guide, [docs/developer.md](docs/developer.md) (threads, conventions, recipes). [BUILDING.md](BUILDING.md) shows how to build.

## Where new code goes

- **Logic in portable headers with tests.** Calculations, protocol handling, data processing and anything else that can run without Windows, ImGui or OpenCV goes in a header like `TestCalc.h` or `ExtensometerFilter.h`, with a unit test in `tests/` and a step in `.github/workflows/tests.yml`. The GUI files only draw and pass on user input.
- **Threads:** never call ImGui outside the GUI thread, and never share a `std::string` or a pointer to a tab between threads (see *Threads* in the developer guide).
- **Colours:** take them from the theme (`CurrentThemePalette()`), never fixed values; `tests/theme_palette_test.cpp` checks the contrast.
- **Settings:** new keys are read as optional with a default, so older settings files still load.
- **Third-party code:** Dear ImGui, ImPlot and the other files in the repository root are upstream copies without changes. A new library needs a license that allows this project's MIT license, and an entry in [LICENSE](LICENSE).

## Pull requests

1. Branch from `main`, keep the change focused, and describe *why* in the pull request.
2. **Builds without new warnings** in Visual Studio (`/W4`, Debug and Release x64).
3. **The tests pass** (*Actions* > *Tests* runs them on every push). New logic comes with a test.
4. **Tested on the machine, or says it was not.** Anything that touches the serial protocol, movement, the load safety or the extensometer needs a test on a real LR 10K before it is merged; say in the pull request who tested it and how (or test with *LR 10K Simulation* and say so). A serial log (*Log serial traffic to file*) helps the reviewer.
5. **Docs updated:** the user guide ([docs/index.md](docs/index.md)) for what users see, ARCHITECTURE.md for new files, and Doxygen comments (`@brief`, `@param`) in the code. The *Docs* workflow lists Doxygen warnings.
6. The version (`LR10KAppVersion` in `common.h`) is raised by the maintainers when a release is made.

## Reporting a safety problem

If the software can move the machine when it should not, fails to stop it, or the load safety does not work: stop using that version, and open an issue starting with **[Safety]**, with the steps and a serial log if you have one. These are fixed first.

## Code of conduct

Be respectful and constructive. This is a teaching project: questions from students are welcome, and reviews should explain, not just reject.
