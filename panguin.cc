#include "panguinOnline.hh"
#include "CLI11.hpp"
#include <TApplication.h>
#include <TString.h>
#include <TROOT.h>
#include <TSystem.h>
#include <iostream>
#include <ctime>
#include <string>

#define PANGUIN_VERSION "1.0"

using namespace std;

clock_t tStart;
void online( const OnlineConfig::CmdLineOpts& opts );

int main( int argc, char** argv )
{
  tStart = clock();
  string cfgfile{"default.cfg"};
  string plotfmt, imgfmt;
  string cfgdir, pltdir, imgdir;
  int run{0};
  int verbosity{0};
  bool printonly{false};
  bool saveImages{false};

  cout << "Starting processing arg. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  CLI::App cli("panguin: configurable ROOT data visualization tool");

  cli.add_option("-f,--file", cfgfile,
                 "Job configuration file")
    ->capture_default_str()->type_name("<file name>");
  cli.add_option("-r,--run", run,
                 "Run number")
    ->type_name("<run number>");
  cli.add_option("-v,--verbosity", verbosity,
                 "Set verbosity level (>=0)")
    ->type_name("<level>");
  cli.add_option("-C,--config-dir", cfgdir,
                 "Configuration directory")
    ->type_name("<dir>");
  cli.add_flag("-P,-b,--batch", printonly,
               "No GUI. Save plots to summary file(s)");
  cli.add_option("-E,--plot-format", plotfmt,
                 "Plot format (pdf, png, jpg ...)")
    ->type_name("<fmt>");
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
  cli.set_version_flag("-V,--version", PANGUIN_VERSION);

  CLI11_PARSE(cli, argc, argv);

  if( saveImages ) {
    printonly = true;
    if( imgdir.empty() )
      imgdir = pltdir;
  }

  if( verbosity <= 0 ) {
    verbosity = 0;
    cout << "Verbosity level set to " << verbosity << endl;
    cout << "Job config file: " << cfgfile << endl;
    cout << "Run number: " << run << endl;
    cout << "Config dir: " << cfgdir << endl;
  } else if( verbosity > 0 ) {
    cout << cli.config_to_str(true, false);
  }

  cout << "Finished processing args. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;


  if( !gSystem->AccessPathName("./rootlogon.C") ) {
    gROOT->ProcessLine(".x rootlogon.C");
  }

  if( !gSystem->AccessPathName("~/rootlogon.C") ) {
    gROOT->ProcessLine(".x ~/rootlogon.C");
  }

  TApplication theApp("panguin2", &argc, argv, nullptr, -1);
  online(OnlineConfig::CmdLineOpts{
    cfgfile, cfgdir, plotfmt, imgfmt, pltdir, imgdir, run, verbosity,
    printonly, saveImages}
  );
  theApp.Run();

  cout << "Done. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  return 0;
}


void online( const OnlineConfig::CmdLineOpts& opts )
{

  if( opts.printonly ) {
    if( !gROOT->IsBatch() ) {
      gROOT->SetBatch();
    }
  }

  cout << "Starting processing cfg. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  OnlineConfig fconfig(opts);
  if( !fconfig.ParseConfig() )
    gApplication->Terminate();

  TString macropath = gROOT->GetMacroPath();
  macropath += ":./macros";   // for backward compatibility
  TString guidir = fconfig.GetGuiDirectory();
  if( !guidir.IsNull() )
    macropath = ".:" + guidir + ":" + macropath;
  gROOT->SetMacroPath(macropath);

  if( opts.run != 0 )
    fconfig.OverrideRootFile(opts.run);

  cout << "Finished processing cfg. Init OnlineGUI. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  new OnlineGUI(std::move(fconfig));

  cout << "Finished init OnlineGUI. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

}
