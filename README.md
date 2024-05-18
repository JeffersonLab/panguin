# PANGUIN

Panguin generates plots from a ROOT file, either interactively in a GUI window
or by writing graphics files in batch mode. Plots are defined in a configuration
file and can come from 1D, 2D, or 3D histograms, tree variables, or user-defined
macros.

## Building

The main prerequisites for building the program are a C++11 compiler and ROOT 
version 6. Additionally, linking with the Hall A
[analyzer](https://github.com/JeffersonLab/analyzer)
is supported (but optional) so that analyzer-specific objects in ROOT files can
be interpreted, for example the event header branch.

The build system is a standard CMake setup. CMake version 3 is required. In the 
source directory of Panguin, do
```
source /path/to/ROOT/bin/thisroot.sh # or "module load root"
export ANALYZER=/path/to/analyzer    # or "module load analyzer". Optional.
cmake -S . -B build 
cmake --build build -j4
```
After this, you should have a working executable in the `build` directory.
To test, do `./build/panguin -h` to show brief usage help.

## Usage and command line options
Running without arguments will load the macros/default.cfg macro and run that. 

A description of available command line options follows

### -f, --config-file \<file name\>
```
./build/panguin -f path/to/nameOfAna.cfg
```
Read the given configuration file.

### -r, --run \<run number\>
```
./build/panguin -r <number>
```

Define the run number for which plots should be generated. This run number will
be substituted into the file name defined with the `protorootfile` command in
the configuration file to generate the ROOT file name. Example:

```
panguin -r 1234 -f myconfig.conf
```
where in `myconfig.conf`
```
protorootfile exp_replay_%R.root
```
will be translated to the ROOT file name `exp_replay_1234.root`.
See later for a full description of `protorootfile`. 

### -P, -b, --batch
```
./build/panguin -P
```
Switches panguin to batch mode. Instead of showing the generated plots in a GUI
window, graphics are written to files. By default, a single PDF file
named `summaryPlots_<run number>_<configuration name>.pdf` is generated, for
example `summaryPlots_1234_myconfig.pdf`. The output file name and file format
is configurable; see the description of `protoplotfile` later.

### -v, --verbosity \<level\>
```
./build/panguin -v 2
```
This will run with a verbosity level of N (higher is more noisy).

### -R, --root-file \<file name\>

Directly specify the input ROOT file.

This option will be ignored, and a warning will be printed, if the run number is
also specified with `-r`.

Panguin will attempt to extract the run number from the given ROOT file name
using a common pattern (regular expression _[0-9]{4,5}[\\._], i.e.
a 4- or 5-digit number preceded by an underscore and followed by either a 
dot or an underscore). If the run number cannot be determined in this way,
it will be omitted from the generated plots.

### -G, --goldenroot-file \<file name\>

Name of ROOT file with reference plots. If the file is not found, a warning 
is printed, and no reference plots are generated.

### -E, --plot-format \<fmt\>

Sets the output plot file format. The default is `pdf`. Also supported
are `png`, `gif`, `jpg`, `ps` and `eps`. For formats other than `pdf`,
one output file per page is generated, and the file name is
generated from the pattern specified by the `protoplotpagefile`
command (see later).

### -C, --config-path, --config-dir \<path\>

Search for configuration files and plot macros in the given directory or
directory path (= list of directories separated by colons). The current working
directory is always searched. The location where the main configuration
file is actually found is automatically added to the search path. Example

```
panguin -f ../configs/myplots.conf -C ~/panguin/otherconfigs
```
will search for `myplots.conf` in `../configs` and
`~/panguin/otherconfigs/../configs` = `~/panguin/configs`.
If `myplots.conf` is found in 
`~/panguin/configs`, then the new search path for additional configuration 
files and macros will be `.:~/panguin/configs:~/panguin/otherconfigs`.

The contents of the environment variable `PANGUIN_CONFIG_PATH` are
automatically appended to the configuration directory search path
(even if --config-dir isn't given on the command line). It is usually more 
convenient to set this variable instead of using the command line option.

### --root-dir \<path\>

Specifies the search directory or path for ROOT files, similar to
--config-dir.

The contents of the environment variable `ROOTFILES` are automatically
appended to this path (even if --root-dir isn't given on the command line).

### -O, --plots-dir \<dir\>

Specifies the directory where summary plots should be written.
This directory will be created if it does not exist, permissions allowing.
The value specified here will be ignored if `protoplotfile` or
`protoplotpagefile` are absolute paths.

### -I, --images

Save each individual plots as an image file. This option automatically
enables batch mode (-P). Summary plots continue to be generated in addition
to the individual image files. The image file name patterns can be
customized with the `protoimagefile` and `protomacroimagefile` commands
in the configuration file.

### -F, --image-format \<fmt\>

Define the file format for individual image files. The default is `png`.
See the descrption of --plot-format.

### -H, --images-dir \<dir\>

Like --plots-dir, defines the directory where individual image files are
written. The default is the same directory as --plots-dir. 
The value specified here will be ignored if `protoimagefile` or
`protomacroimagefile` are absolute paths.

### --inspect \<file name\>

Prints the list of objects in the given ROOT file. No configuration file will be
read, and all other options except --verbosity will be ignored.
The printout is grouped by histograms, trees, and other objects, if any.
If verbosity > 0, details of histograms and the variables contained in the
tree(s) will be listed.
If a Podd "Run_Data" object (inheriting from THaRunBase) is found, the run
metadata will also be printed.

### -V, --version

Print program version and exit.

## Online monitor

With the **watchfile** option enabled the GUI will reload the file every few
seconds and will redraw the current canvas (for default usage please look at
defaultOnline.cfg).

The process to run the online monitor goes as follows: 
a) Run the ET connected japan output:
```
cd mainJapanDir
build/qwparity --config prex.conf --add-config online_apar.conf
```

This will connect to the ET system and analyze events as they come through the
ET system. It will update the output file every ~30s. The output file will be
placed in the ROOTFILES directory and will have the following pattern: ''
prexALL_999999.adaq*.root''. This file can be accessed from the prompt or
through panguin. b) Run panguin:

```
cd mainJapanDir/pangin
build/panguin -r 999999 -f macros/defaultOnline.cfg
```

The configuration can be changed with whatever you need, but it must have the 
"watchfile" option set.

## Configuration file options

The configuration file consists of two sections, the "prologue" where 
various global parameters are set, followed by the "page definitions" that 
define the plots and drawing options for each output page

Most of the commands in the prologue can also be given as command line options,
documented earlier. If an option is present both on the command line and in the
configuration file, the value from the command line takes precedence, and a
warning is printed. Exceptions are --run-number and --config-dir, which can only
be specified on the command line, and --root-dir, where the command line and
configuration file values are cumulative (command line first). A number of
options can only be specified in the configuration file (see below).

Example configuration files can be found in macros/default.cfg and 
macros/defaultOnline.cfg.

References to environment variables and the home directory shorthand "~" in any 
command arguments containing file or directory names will be expanded. Example
```
rootfile $EXP_DIR/rootfiles/testdata.root
```
will replace `$EXP_DIR` with the value of that environment variable, if set. 
(If unset, `"$EXP_DIR"` is left unchanged.) Similarly, `plotsdir 
~/panguin/plots` will expand to `plotsdir /home/user/panguin/plots` etc.

Following is the list of options recognized in the prologue.

### Configuration file handling

- **include \<file name\>** executes the commands in the given file as part of
  the current configuration. This is especially useful for defining common
  options for different configurations, like `protorootfile`, `goldenrootfile`,
  `plotFormat` etc. If `<file name>` is not an absolute path, it will be 
  searched for in the same directory as the current file and all directories 
  given with --config-dir and `$PANGUIN_CONFIG_PATH`, as explained earlier.

### Input file selection

- **rootfile \<file name\>** selects the input ROOT file. Equivalent to 
  --root-file.
- **protorootfile \<file name pattern\>** specifies a pattern for the input ROOT
  file. The file name pattern may contain environment variables, to be be
  expanded at run time, as well as the placeholder **%R**, which will be
  replaced with the run number (given with -r or --run-number). For backward
  compatibility, the string "XXXXX" (five "X" characters) is equivalent to %R.

  Multiple **protorootfile** commands may be given. The corresponding patterns
  will be tried in the order in which they are defined. This enables, for
  example, finding ROOT files that differ in a common prefix, such as a DAQ
  configuration. This works well if run numbers are unique, but text parts of
  the file name may vary.
- **goldenrootfile \<file name\>** selects a ROOT file containing comparison 
  plots (reference spectra) to help spot problems with the current run.
  Reference plots will be overlaid onto the current spectra with a green hatch 
  pattern. If the specified ROOT file is not found, a warning is printed, and no
  comparison plots are generated. Equivalent to --goldenroot-file.
- **rootfilespath \<directory path\>** specifies a path for searching for ROOT
  files (whether specified with `rootfile`, `protorootfile`, or
  `goldenrootfile`). Equivalent to --root-dir. If both --root-dir and
  `rootfilespath` are given, values from both are concatenated. Additionally,
  the contents of environment variable `$ROOTFILES` is added to this search
  path.

### Output file options

The following options control the format, location, and names of summary plot 
output files.

- **plotsdir \<directory\>** sets the directory where the summary plots will be
  created. Equivalent to --plots-dir.
- **plotFormat \<format\>** option allows you to select the format of the plots
  (png, gif, pdf). Default is "pdf". Equivalent to --plot-format.
- **protoplotfile \<file name pattern\>** defines the file name for summary plot
  files in PDF format (i.e. all pages in a single file). The file name may
  contain a directory component. Any of the following placeholders may be
  included, which will be replaced as indicated:
  * **%R**: run number (given with -r or --run-number)
  * **%C**: main configuration file name (given with -f or --config-file)
    without any leading directory components and without any file extension
    (like .cfg).

  The file name should end with ".pdf". If it does not, the existing extension
  will be removed and replaced with ".pdf". The default is 
  `summaryPlots_%R_%C.pdf`.
- **protoplotpagefile \<file name pattern\>** defines the file name for 
  summary plot files in non-PDF format, for which one file is generated per 
  page. The following placeholders are supported
  * **%R**: run number (given with -r or --run-number)
  * **%C**: main configuration file name (given with -f or --config-file)
    without any leading directory components and without any file extension
    (like .cfg).
  * **%P**: The current page number. 
  * **%E**: The selected `plotFormat`.

  The file name pattern should always end with ".%E", otherwise the 
  extension corresponding to the current format will be subsituted 
  automatically. The default is `summaryPlots_%R_page%P_%C.%E`.

### Image file options

These options control the format, location, and names of individual image files

- **imagesdir \<directory\>** Like `plotsdir` except for image file. If
  `plotsdir` is given but `imagesdir` is not, `imagedir` is set to `plotsdir`.
  Equivalent to --images-dir.
- **imageFormat \<format\>** Like `plotFormat` except for images. Defaults 
  is "png". Equivalent to --image-format.
- **protoimagefile \<file name pattern\>** File name pattern for image files 
  generated from histograms or tree variables. Supports the following 
  placeholders
  * **%R**: run number (given with -r or --run-number)
  * **%C**: main configuration file name (given with -f or --config-file)
    without any leading directory components and without any file extension
    (like .cfg).
  * **%V**: The histogram or tree variable name.
  * **%P**: The current page number.
  * **%D**: The current pad number within the page canvas. The combination 
    of page number and pad number uniquely identifies an image for a given 
    configuration.
  * **%F**: The selected `imageFormat`.

  This pattern should always end with ".%F", similar to `protoplotpagefile`. 
  The default is `hydra_%R_%V_%C.%F`.
- **protomacroimagefile \<file name pattern\>** Like `protoimagefile` except
  that this pattern is used for images generated from user macros.

  The `%V`  placeholder is replaced with the name of the macro minus any
  directory component and extension. For example, if the user macro is
  `mymacros/TreeDraw.C+(arguments)`, `%V` would be replaced with
  `"TreeDraw"`.

  Like `protoimagefile`, this pattern should always end with ".%F". The default
  is `hydra_%R_page%P_pad%D_%C.%F`.

### watchfile option 
See online monitor above.

### Non-standard GUI color

- **guicolor** followed by the string of a color like (white, red, blue) allows
  you to set the border color of the gui

### Histogram binning options

- **2DbinsX** or **2DbinsY** followed by a number; for 2D histograms this option
  allows you to set the number of bins (default ROOT is 40 bins)

### Cuts

- **definecut** followed by a string (with no spaces and no quotation marks);
  this allows you to group any number of cuts (using the standard TTree cut
  syntax) and give it a simple name to be used later.

## Page definitions

- **newpage \<N\> \<M\>** creates a new page with N columns and M rows,
  corresponding to NxM drawing pads on a ROOT canvas.

  This command must always be the first command for a new page. The first
  `newpage`  command in the configuration file ends the prologue and starts the
  page definition part of the file. Prologue commands are no longer accepted
  after `newpage`. A page definition ends with the next `newpage`
  command or with the end of the file.
- **title** followed by any number of strings (spaces can be included) allows
  you to set the page title

### Plot definitions

Within each page, a plot is defined by simply stating one or more variable names
followed by drawing options. Panguin will search the input ROOT file for
histograms and branch names in TTrees for a match with these names.

The number of plot definitions should match the number of drawing pads of the 
page defined by the preceding `newpage` command.

The following syntax is regocnized for histogram and tree variables

- **var1D** will make a 1D histogram of var1D (if found in any of the TTrees).
  If the variable matches a histogram name (1D, 2D or 3D), the histogram 
  will be drawn.
- **var1:var2** draw a 2D histogram from given tree variables
- **var1:var2 CodaEventNumber>10** same as above with the extra cut based on
  variables available in the TTree

Any of the above plot definitions may optionally include any 
combination of the following modifiers

- **-drawopt \<options\>** set draw options for histograms and tree variables, 
  like TH1::Draw(options)
- **-title "\<string\>"** set plot title, enclosed in double quotes. This is 
  separate from the page title described in the previous section.
- **-tree \<name\>** select TTree from which to get tree variable
- **-grid** set "grid" option
- **-logx**, **-logy**, **-logz"**  draw with log x,y,z axis
- **-nostat** disable stats box
- **-noshowgolden** don't draw "golden" histogram even if `goldenrootfile` is 
  defined

Additionally, any plots based on tree variables may include a cut name 
defined with `definecut` to select a subset of tree entries.

For plots generated by macros, use this syntax:

- **macro someMacro.C** This must create only a single plot. The macro code may
  be compiled by adding a "+" or "++" after the file name eextension,
  e.g. `someMacro.C+`. (In this case, don't forget to add all
  necessary `#include` statements in the macro.) Arbitrary arguments may be
  given to the macro, e.g. `someMacro.C(1,"myresult")`.

This command does not take additional options. It is assumed that details of the
plot layout are defined within the macro. If the macro modifies global
parameters such as the color palette, font sizes, etc., it should save the prior
state and restore it before exiting.
