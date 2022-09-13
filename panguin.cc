#include "panguinOnline.hh"
#include "CLI11.hpp"
#include <TApplication.h>
#include <TString.h>
#include <TROOT.h>
#include <TSystem.h>
#include <iostream>
#include <ctime>
#include <string>
#include <stdexcept>

#define PANGUIN_VERSION "Panguin version 2.2 (31-Aug-2022)"

using namespace std;

void online( const OnlineConfig::CmdLineOpts& opts );

int main( int argc, char** argv )
{
  string cfgfile{"default.cfg"}, rootfile;
  string plotfmt, imgfmt;
  string cfgdir, rootdir, pltdir, imgdir;
  int run{0};
  int verbosity{0};
  bool printonly{false};
  bool saveImages{false};

  try {
    CLI::App cli("panguin: configurable ROOT data visualization tool");

    cli.add_option("-f,--config-file", cfgfile,
                   "Job configuration file")
      ->capture_default_str()->type_name("<file name>");
    cli.add_option("-r,--run", run,
                   "Run number")
      ->type_name("<run number>");
    cli.add_option("-R,--root-file", rootfile,
                   "ROOT file to process")
      ->type_name("<file name>");
    cli.add_flag("-P,-b,--batch", printonly,
                 "No GUI. Save plots to summary file(s)");
    cli.add_option("-E,--plot-format", plotfmt,
                   "Plot format (pdf, png, jpg ...)")
      ->type_name("<fmt>");
    cli.add_option("-C,--config-dir", cfgdir,
                   "Search path for configuration files & macros "
                   "(\":\"-separated)")
      ->type_name("<path>");
    cli.add_option("--root-dir", rootdir,
                   "ROOT files search path (\":\"-separated)")
      ->type_name("<path>");
    cli.add_option("-O,--plots-dir", pltdir,
                   "Output directory for summary plots")
      ->type_name("<dir>");
    cli.add_flag("-I,--images", saveImages,
                 "Save individual plots as images (implies -P)");
    cli.add_option("-F,--image-format", imgfmt,
                   "Image file format (png, jpg ...)")
      ->type_name("<fmt>");
    cli.add_option("-H,--images-dir", imgdir,
                   "Output directory for individual images (default: plots-dir)")
      ->type_name("<dir>");
    cli.add_option("-v,--verbosity", verbosity,
                   "Set verbosity level (>=0)")
      ->type_name("<level>");
    cli.set_version_flag("-V,--version", PANGUIN_VERSION);

    CLI11_PARSE(cli, argc, argv);

    if( saveImages ) {
      printonly = true;
      if( imgdir.empty() )
        imgdir = pltdir;
    }

    if( verbosity <= 0 ) {
      verbosity = 0;
    } else if( verbosity > 0 ) {
      cout << cli.config_to_str(true, false);
    }

    if( !gSystem->AccessPathName("./rootlogon.C") ) {
      gROOT->ProcessLine(".x rootlogon.C");
    }

    if( !gSystem->AccessPathName("~/rootlogon.C") ) {
      gROOT->ProcessLine(".x ~/rootlogon.C");
    }

    TApplication theApp("panguin2", &argc, argv, nullptr, -1);
    online({cfgfile, cfgdir, rootfile, rootdir, plotfmt, imgfmt, pltdir,
            imgdir, run, verbosity, printonly, saveImages});
    theApp.Run();

  } catch ( const exception& e ) {
    cerr << "Error while running panguin: " << e.what() << endl;
    return 1;
  }

  return 0;
}


void online( const OnlineConfig::CmdLineOpts& opts )
{

  if( opts.printonly ) {
    if( !gROOT->IsBatch() ) {
      gROOT->SetBatch();
    }
  }

  OnlineConfig fconfig(opts);
  if( !fconfig.ParseConfig() )
    gApplication->Terminate();

  TString macropath = gROOT->GetMacroPath();
  macropath += ":./macros";   // for backward compatibility
  TString guipath = fconfig.GetConfFilePath();
  if( !guipath.IsNull() )
    macropath = ".:" + guipath + ":" + macropath;
  gROOT->SetMacroPath(macropath);

  new OnlineGUI(std::move(fconfig));
}
