#include "panguinOnline.hh"
#include "CLI11.hpp"
#include <TApplication.h>
#include <TString.h>
#include <TROOT.h>
#include <TSystem.h>
#include <iostream>
#include <ctime>
#include <string>

using namespace std;

clock_t tStart;
void online( const OnlineConfig::CmdLineOpts& opts );

int main( int argc, char** argv )
{
  tStart = clock();
  string cfgfile{"default.cfg"};
  string cfgdir;
  string plotfmt{"pdf"};
  UInt_t run{0};
  bool printonly{false};
  bool saveImages{false};
  int verbosity{0};

  TString macropath = gROOT->GetMacroPath();
  macropath += ":./macros";
  gROOT->SetMacroPath(macropath.Data());

  cout << "Starting processing arg. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  CLI::App cli("panguin: configurable ROOT data visualization tool");

  cli.add_option("-f,--file", cfgfile, "Job configuration file")
    ->capture_default_str()->type_name("<file name>");
  cli.add_option("-r,--run", run, "Run number")
    ->type_name("<run number>");
  cli.add_option("-v,--verbosity", verbosity, "Set verbosity level (>=0)")
    ->type_name("<level>");
  cli.add_option("-C,--config-dir", cfgdir, "Configuration directory")
    ->type_name("<dir>");
  cli.add_option("-F,--plot-format", plotfmt, "Plot format (pdf, png, jpg ...)")
    ->capture_default_str()->type_name("<fmt>");
  cli.add_flag("-P,,-b,--batch", printonly, "No GUI. Write plots to file");
  cli.add_flag("-I,--images", saveImages, "Save plots as png images");
  cli.set_version_flag("-V,--version", "1.0");

  CLI11_PARSE(cli, argc, argv);

  if( verbosity > 1 ) {
    cout << cli.config_to_str(true, false);
  }

  if (verbosity < 0)
    verbosity = 0;
  cout << "Verbosity level set to "<<verbosity<<endl;

  cout<<"Finished processing arg. Time passed: "
      <<(double) ((clock() - tStart)/CLOCKS_PER_SEC)<<" s!"<<endl;

  cout << "Job config file: " << cfgfile << endl;
  cout << "Run number: " << run << endl;
  cout << "Config dir: " << cfgdir << endl;

  if( !gSystem->AccessPathName("./rootlogon.C") ) {
    gROOT->ProcessLine(".x rootlogon.C");
  }

  if( !gSystem->AccessPathName("~/rootlogon.C") ) {
    gROOT->ProcessLine(".x ~/rootlogon.C");
  }

  TApplication theApp("panguin2", &argc, argv, nullptr, -1);
  online(OnlineConfig::CmdLineOpts{
    cfgfile, cfgdir, plotfmt, run, verbosity, printonly, saveImages});
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

  if( opts.run != 0 )
    fconfig.OverrideRootFile(opts.run);

  cout << "Finished processing cfg. Init OnlineGUI. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

  new OnlineGUI(std::move(fconfig));

  cout << "Finished init OnlineGUI. Time passed: "
       << (double) ((clock() - tStart) / CLOCKS_PER_SEC) << " s!" << endl;

}
