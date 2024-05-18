#include "panguinOnline.hh"
#include "CLI11.hpp"
#include <TApplication.h>
#include <TString.h>
#include <TROOT.h>
#include <TSystem.h>
#include <iostream>
#include <stdexcept>
#include <memory>

#define PANGUIN_VERSION "Panguin version 2.8 (17-May-2024)"

using namespace std;

unique_ptr<OnlineGUI> online( const OnlineConfig::CmdLineOpts& opts );

int main( int argc, char** argv )
{
  string cfgfile{"default.cfg"}, rootfile, goldenfile, scanfile;
  string plotfmt, imgfmt;
  string cfgdir, rootdir, pltdir, imgdir;
  int run{0};
  int verbosity{0};
  bool printonly{false};
  bool saveImages{false};

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
  cli.add_option("-G,--goldenroot-file", goldenfile,
                 "Reference ROOT file")
    ->type_name("<file name>");
  cli.add_flag("-P,-b,--batch", printonly,
               "No GUI. Save plots to summary file(s)");
  cli.add_option("-E,--plot-format", plotfmt,
                 "Plot format (pdf, png, jpg ...)")
    ->type_name("<fmt>");
  cli.add_option("-C,--config-path,--config-dir", cfgdir,
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
  cli.add_option("--inspect", scanfile,
                 "List objects in given ROOT file")
    ->type_name("<file name>");
  cli.set_version_flag("-V,--version", PANGUIN_VERSION);

  CLI11_PARSE(cli, argc, argv)

  if( saveImages ) {
    printonly = true;
    if( imgdir.empty() )
      imgdir = pltdir;
  }

  if( verbosity > 0 )
    cout << cli.config_to_str(true, false);
  else
    verbosity = 0;

  if( !gSystem->AccessPathName("./rootlogon.C") ) {
    gROOT->ProcessLine(".x rootlogon.C");
  }

  if( !gSystem->AccessPathName("~/rootlogon.C") ) {
    gROOT->ProcessLine(".x ~/rootlogon.C");
  }

  TApplication theApp("panguin", &argc, argv, nullptr, -1);
  try {
    if( scanfile.empty() ) {
      auto gui
        = online({cfgfile, cfgdir, rootfile, goldenfile, rootdir, plotfmt,
                  imgfmt, pltdir, imgdir, run, verbosity, printonly,
                  saveImages});
      if( gui ) {
        if( gui->IsPrintOnly() )
          gui->PrintPages();
        else
          theApp.Run(true);
      }
    } else {
#if __cplusplus >= 201402L
      auto gui = make_unique<OnlineGUI>();
#else
      auto gui = unique_ptr<OnlineGUI>(new OnlineGUI);
#endif
      gui->InspectRootFile(scanfile);
    }

  } catch ( const exception& e ) {
    cerr << "Error while running panguin: " << e.what() << endl;
    theApp.Terminate(1);
    return 1;
  }

  return 0;
}


unique_ptr<OnlineGUI> online( const OnlineConfig::CmdLineOpts& opts )
{

  if( opts.printonly ) {
    if( !gROOT->IsBatch() ) {
      gROOT->SetBatch();
    }
  }

  OnlineConfig fconfig(opts);
  if( !fconfig.ParseConfig() )
    return nullptr;

  TString macropath = gROOT->GetMacroPath();
  macropath += ":./macros";   // for backward compatibility
  TString guipath = fconfig.GetConfFilePath();
  if( !guipath.IsNull() )
    macropath = ".:" + guipath + ":" + macropath;
  gROOT->SetMacroPath(macropath);

#if __cplusplus >= 201402L
  return make_unique<OnlineGUI>(std::move(fconfig));
#else
  return unique_ptr<OnlineGUI>(new OnlineGUI(std::move(fconfig)));
#endif
}
