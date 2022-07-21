#include "panguinOnlineConfig.hh"
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <utility>
#include <dirent.h>
#include <cmath>

using namespace std;

static string DirnameStr( string path )
{
  auto pos = path.rfind('/');
  if( pos == string::npos )
    return ".";
  if( pos > 0 )
    path.erase(pos);
  else if( path.length() > 1 )
    path.erase(pos + 1);
  return path;
}

// Constructor.  Without an argument, use default config
OnlineConfig::OnlineConfig()
  : OnlineConfig("default.cfg")
{}

// Constructor.  Takes the config file name as the only argument.
//  Loads up the configuration file, and stores its contents for access.
OnlineConfig::OnlineConfig( const string& config_file_name )
  : OnlineConfig(CmdLineOpts{config_file_name}) {}

OnlineConfig::OnlineConfig( const CmdLineOpts& opts )
  : confFileName(opts.cfgfile)
  , fFoundCfg(false)
  , fMonitor(false)
  , fVerbosity(opts.verbosity)
  , hist2D_nBinsX(0)
  , hist2D_nBinsY(0)
  , fRunNumber(opts.run)
  , fPrintOnly(opts.printonly)
  , fSaveImages(opts.saveimages)
  , fPlotFormat(opts.plotfmt)
{
  // Pick up config file directory/path form environment.
  // A config dir given on the command line has preference.
  string cfgpath = opts.cfgdir;
  if( !cfgpath.empty() )
    cfgpath += ":";
  cfgpath += ".";
  const char* env_cfgdir = getenv("PANGUIN_CONFIG_DIR");
  if( env_cfgdir ) {
    cfgpath += string(":") + env_cfgdir;
  }
  if( fVerbosity > 0 )
    cout << "config file path = " << cfgpath << endl;

  string trypath(confFileName);
  ifstream infile(trypath);
  if( !infile ) {
    istringstream istr(cfgpath);
    while( getline(istr, trypath, ':') ) {
      fConfFilePath = trypath;
      trypath += "/" + confFileName;
      infile.clear();
      infile.open(trypath);
      if( infile )
        break;
    }
  } else {
    fConfFilePath = DirnameStr(trypath);
  }
  if( !infile ) {
    cerr << "OnlineConfig() ERROR: no file " << confFileName << endl;
    cerr << "You need a configuration to run.  Ask an expert." << endl;
    fFoundCfg = false;
  } else {
    clog << "GUI Configuration loading from " << trypath << endl;
    confFileName = std::move(trypath);
    fFoundCfg = true;
  }

  if( fFoundCfg ) {
    LoadFile(infile);
    infile.close();
  }

}

int OnlineConfig::LoadFile( std::ifstream& infile )
{
  // Reads in the Config File, and makes the proper calls to put
  //  the information contained into memory.

  if( !infile )
    return 1;

  const char comment = '#';
  vector<string> strvect;
  string sinput, sline;
  while( getline(infile, sline) ) {
    if( sline.find(comment) != string::npos ) continue;
    istringstream istr(sline);
    string field;
    while( istr >> field )
      strvect.push_back(std::move(field));
    if( !strvect.empty() )
      sConfFile.push_back(std::move(strvect));
    strvect.clear();
  }

  if( fVerbosity >= 1 ) {
    cout << "OnlineConfig::LoadFile()\n";
    for( uint_t ii = 0; ii < sConfFile.size(); ii++ ) {
      cout << "Line " << ii << endl << "  ";
      for(UInt_t jj=0; jj<sConfFile[ii].size(); jj++)
	cout << sConfFile[ii][jj] << " ";
      cout << endl;
    }
  }

  cout << "     " << sConfFile.size() << " lines read from "
       << confFileName << endl;

  return 0;
}

static int ExtractRunNumber( const string& filename )
{
  // Extract run number from a file name structured as
  //   experiment_optionaltext_runnumber.dat
  string temp = filename.substr(filename.rfind('_') + 1);
  return stoi(temp.substr(0, temp.rfind('.')));
}

static string ReplaceAll( string str, const string& ostr, const string& nstr )
{
  size_t pos = 0, ol = ostr.size(), nl = nstr.size();
  while( (pos += nl) || pos == 0 ) {
    pos = str.find(ostr, pos);
    if( pos == string::npos )
      break;
    str.replace(pos, ol, nstr);
  }
  return str;
}

bool OnlineConfig::ParseConfig()
{
  //  Goes through each line of the config [must have been LoadFile()'d]
  //   and interprets.

  if( !fFoundCfg ) {
    return false;
  }

  uint_t command_cnt = 0;
  // If statement for each high level command (cut, newpage, etc)
  for( uint_t i = 0; i < sConfFile.size(); i++ ) {
    // "newpage" command
    if( sConfFile[i][0] == "newpage" ) {
      // sConfFile[i] is first of pair
      for(j=i+1;j<sConfFile.size();j++) {
	if(sConfFile[j][0] != "newpage") {
	  // Count how many commands within the page
	  command_cnt++;
	} else break;
      }
      pageInfo.push_back(make_pair(i,command_cnt));
      i += command_cnt;
      command_cnt = 0;
    }
    if( sConfFile[i][0] == "watchfile" ) {
      fMonitor = true;
    }
    if( sConfFile[i][0] == "2DbinsX" ) {
      hist2D_nBinsX = stoi(sConfFile[i][1]);
    }
    if( sConfFile[i][0] == "2DbinsY" ) {
      hist2D_nBinsY = stoi(sConfFile[i][1]);
    }
    if( sConfFile[i][0] == "definecut" ) {
      if( sConfFile[i].size() > 3 ) {
        cerr << "cut command has too many arguments" << endl;
        continue;
      }
      cutList.emplace_back(sConfFile[i][1], sConfFile[i][2]);
    }
    if( sConfFile[i][0] == "rootfile" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: rootfile command does not have the "
             << "correct number of arguments"
             << endl;
        continue;
      }
      if( !rootfilename.empty() ) {
        cerr << "WARNING: too many rootfile's defined. "
             << " Will only use the first one."
             << endl;
        continue;
      }
      rootfilename = sConfFile[i][1];
      fRunNumber = ExtractRunNumber(sConfFile[i][1]);
    }
    if( sConfFile[i][0] == "goldenrootfile" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: goldenfile command does not have the "
             << "correct number of arguments"
             << endl;
        continue;
      }
      if( !goldenrootfilename.empty() ) {
        cerr << "WARNING: too many goldenrootfile's defined. "
             << " Will only use the first one."
             << endl;
        continue;
      }
      goldenrootfilename = sConfFile[i][1];
    }
    if( sConfFile[i][0] == "protorootfile" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: protorootfile command does not have the "
             << "correct number of arguments"
             << endl;
        continue;
      }
      if( !protorootfile.empty() ) {
        cerr << "WARNING: too many protorootfile's defined. "
             << " Will only use the first one."
             << endl;
        continue;
      }
      protorootfile = sConfFile[i][1];
    }
    if( sConfFile[i][0] == "guicolor" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: guicolor command does not have the "
             << "correct number of arguments (needs 1)"
             << endl;
        continue;
      }
      if( !guicolor.empty() ) {
        cerr << "WARNING: too many guicolor's defined. "
             << " Will only use the first one."
             << endl;
        continue;
      }
      guicolor = sConfFile[i][1];
    }
    if( sConfFile[i][0] == "plotsdir" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: plotsdir command does not have the "
             << "correct number of arguments (needs 1)"
             << endl;
        continue;
      }
      if( !plotsdir.empty() ) {
        cerr << "WARNING: too many plotdir's defined. "
             << " Will only use the first one."
             << endl;
        continue;
      }
      plotsdir = sConfFile[i][1];
    }
    if( sConfFile[i][0] == "plotFormat" ) {
      if( sConfFile[i].size() != 2 ) {
        cerr << "WARNING: plotsdir command does not have the "
             << "correct number of arguments (needs 1)"
             << endl;
        continue;
      }
      fPlotFormat = sConfFile[i][1];
    }

  }

  if( fVerbosity >= 3 ) {
    cout << "OnlineConfig::ParseConfig()\n";
    for( uint_t i = 0; i < GetPageCount(); i++ ) {
      cout << "Page " << i << " (" << GetPageTitle(i) << ")"
           << " will draw " << GetDrawCount(i)
           << " histograms." << endl;
    }
  }

  cout << "Number of pages defined = " << GetPageCount() << endl;
  cout << "Number of cuts defined = " << cutList.size() << endl;

  if( fMonitor )
    cout << "Will periodically update plots" << endl;
  if( !goldenrootfilename.empty() ) {
    cout << "Will compare chosen histograms with the golden rootfile: "
         << endl
         << goldenrootfilename << endl;
  }

  return true;

}

const string& OnlineConfig::GetDefinedCut( const string& ident )
{
  // Returns the defined cut, according to the identifier

  static const string nullstr{};

  for( const auto& cut: cutList ) {
    if( cut.first == ident ) {
      return cut.second;
    }
  }
  return nullstr;
}

 vector<string> OnlineConfig::GetCutIdent()
{
  // Returns a vector of the cut identifiers, specified in config
  vector<string> out;

  for(UInt_t i=0; i<cutList.size(); i++) {
    out.push_back(cutList[i].GetName());
  }
  return out;
}

bool OnlineConfig::IsLogy( uint_t page )
{
  // Check if last word on line is "logy"

  uint_t page_index = pageInfo[page].first;
  size_t word_index = sConfFile[page_index].size() - 1;
  if( word_index <= 0 ) return false;
  const string& option = sConfFile[page_index][word_index];
  if( option == "logy" ) {
    cout << endl << "Found a logy!!!" << endl << endl;
    return true;
  }
  if( fVerbosity >= 1 ) {
    cout << "OnlineConfig::IsLogy()     " << option << " " << page_index << " " << word_index
	 << " " << sConfFile[page_index].size() << endl;
    for (Int_t i= 0; i < sConfFile[page_index].size(); i++) {
      cout << sConfFile[page_index][i] << " ";
    }
  }
  return false;
}

pair<uint_t, uint_t> OnlineConfig::GetPageDim( uint_t page )
{
  // If defined in the config, will return those dimensions
  //  for the indicated page.  Otherwise, will return the
  //  calculated dimensions required to fit all histograms.

  pair<uint_t, uint_t> outDim;

  // This is the page index in sConfFile.
  uint_t page_index = pageInfo[page].first;

  uint_t size1 = 2;
  if( IsLogy(page) ) size1 = 3;  // last word is "logy"

  // If the dimensions are defined, return them.
  if( sConfFile[page_index].size() > size1 - 1 ) {
    if( sConfFile[page_index].size() == size1 ) {
      outDim = make_pair(uint_t(stoi(sConfFile[page_index][1])),
                         uint_t(stoi(sConfFile[page_index][1])));
      return outDim;
    } else if( sConfFile[page_index].size() == size1 + 1 ) {
      outDim = make_pair(uint_t(stoi(sConfFile[page_index][1])),
                         uint_t(stoi(sConfFile[page_index][2])));
      return outDim;
    } else {
      cout << "Warning: newpage command has too many arguments. "
           << "Will automatically determine dimensions of page."
           << endl;
    }
  }

  // If not defined, return the "default."
  uint_t draw_count = GetDrawCount(page);
  uint_t dim = lround(sqrt(draw_count + 1));
  outDim = make_pair(dim, dim);

  return outDim;
}

string OnlineConfig::GetPageTitle( uint_t page )
{
  // Returns the title of the page.
  //  if it is not defined in the config, then return "Page #"

  string title;
  uint_t iter_command = pageInfo[page].first + 1;

  for( uint_t i = 0; i < pageInfo[page].second; i++ ) { // go through each command
    if( sConfFile[iter_command + i][0] == "title" ) {
      // Combine the strings, and return it
      for( uint_t j = 1; j < sConfFile[iter_command + i].size(); j++ ) {
        title += sConfFile[iter_command + i][j];
        title += " ";
      }
      if( !title.empty() )
        title.erase(title.size() - 1);
      return title;
    }
  }
  title = "Page ";
  title += to_string(page);
  return title;
}

vector<uint_t> OnlineConfig::GetDrawIndex( uint_t page )
{
  // Returns an index of where to find the draw commands within a page
  //  within the sConfFile vector

  vector<uint_t> index;
  uint_t iter_command = pageInfo[page].first + 1;

  for( uint_t i = 0; i < pageInfo[page].second; i++ ) {
    if( sConfFile[iter_command + i][0] != "title" ) {
      index.push_back(iter_command + i);
    }
  }

  return index;
}

uint_t OnlineConfig::GetDrawCount( uint_t page )
{
  // Returns the number of histograms that have been request for this page
  uint_t draw_count = 0;

  for( uint_t i = 0; i < pageInfo[page].second; i++ ) {
    if( sConfFile[pageInfo[page].first + i + 1][0] != "title" ) draw_count++;
  }

  return draw_count;

}

void OnlineConfig::GetDrawCommand( uint_t page, uint_t nCommand, std::map<string, string>& out_command )
{
  // Returns the vector of strings pertaining to a specific page, and
  //   draw command from the config.
  // Return map<string,string> in out_command:
  //Following options are implemented:
  // 1. "-drawopt" --> set draw options for histograms and tree variables
  // 2. "-title" --> set title, enclose in double quotes
  // 3. "-tree" --> set tree name
  // 4. "-grid" --> set "grid" option
  // 5. "-logx, -logy, -logz" --> draw with log x,y,z axis
  // 6. "-nostat" --> don't show stats box
  // 7. "-noshowgolden" --> don't show "golden" histogram even if goldenrootfile is defined
  // 8. any option not preceded by these indicators is assumed to be a cut or macro expression:
  //what options do we want?
  // all options on one line. First argument assumed to be histogram or tree name (or "macro")

  out_command.clear();

  //vector <string> out_command(6);
  vector<uint_t> command_vector = GetDrawIndex(page);
  uint_t index = command_vector[nCommand];

  if( fVerbosity > 1 ) {
    cout << __PRETTY_FUNCTION__ << "\t" << __LINE__ << endl;
    cout << "OnlineConfig::GetDrawCommand(" << page << ","
         << nCommand << ")" << endl;
  }

  // for(uint_t i=0; i<out_command.size(); i++) {
  //   out_command[i] = "";
  // }

  // First line is the variable
  if( !sConfFile[index].empty() ) {
    out_command["variable"] = sConfFile[index][0];
  }

  if( out_command["variable"] == "macro" && sConfFile[index].size() > 1 ) {

    string macrocmd; //interpret the rest of the line as the macro to execute:
    for( int i = 1; i < sConfFile[index].size(); i++ ) {
      macrocmd += sConfFile[index][i];
    }

    out_command["macro"] = macrocmd;
    return;
  }

  if( out_command["variable"] == "loadmacro" && sConfFile[index].size() > 2 ) {
    out_command["library"] = sConfFile[index][1]; //shared library to load
    out_command["macro"] = sConfFile[index][2]; //macro command to execute
    return;
  }

  if( out_command["variable"] == "loadlib" && sConfFile[index].size() > 1 ) {
    out_command["library"] = sConfFile[index][1]; //shared library to load
  }

  // Now go through the rest of that line..
  for( uint_t i = 1; i < sConfFile[index].size(); i++ ) {
    if( sConfFile[index][i] == "-drawopt" && i + 1 < sConfFile[index].size() ) {
      // if(out_command[2].empty()){
      //   out_command[2] = sConfFile[index][i+1];
      //   i = i+1;
      // } else {
      //   cout << "Error: Multiple types in line: " << index << endl;
      //   exit(1);
      // }
      out_command["drawopt"] = sConfFile[index][i + 1];
      i++;
    } else if( sConfFile[index][i] == "-title" && i + 1 < sConfFile[index].size() ) {
      // Put the entire title, (must be) surrounded by quotes, as one string
      string title;
      if( sConfFile[index][i + 1].front() != '\"' ) {
        cout << "Error: title must be surrounded by double quotes. Page: " << page << "--" << GetPageTitle(page)
             << "\t coomand: " << nCommand << endl;
        exit(1); // FIXME: exit?
      }
      for( auto j = i + 1; j < sConfFile[index].size(); j++ ) {
        string word = sConfFile[index][j];
        if( word == "\"" ) { // single " surrounded by space
          if( title.empty() ) continue;  // beginning "
          else {  // ending "
            i = j;
            break;
          }
        } else if( word.front() == '\"' && word.back() == '\"' ) {
          title += ReplaceAll(word, "\"", "");
          i = j;
          break;
        } else if( word.front() == '\"' ) {
          title = ReplaceAll(word, "\"", "");
        } else if( word.back() == '\"' ) {
          title += " " + ReplaceAll(word, "\"", "");
          i = j;
          break;
        } else if( title.empty() ) {
          title = word;
        } else {
          //  This case uses neither "i = j;" or "break;", because
          //  we want to be able to include all the words in the title.
          //  The title will end before the end of the line only if
          //  it is delimited by quotes.
          title += " " + word;
        }
      }
      if( i == sConfFile[index].size() && sConfFile[index][i - 1].back() != '\"' ) {
        // unmatched double quote
        cout << "Error, unmatched double quote, please check you config file. Quitting" << endl;
        exit(1);
      }

      out_command["title"] = title;

      // if (out_command[3].empty()){
      //   out_command[3] = title;
      // } else {
      //   cout << "Error: Multiple titles in Page: " << page << "--" << GetPageTitle(page).Data() << "\t coomand: " << nCommand << endl;
      //   exit(1);
      // }
    } else if( sConfFile[index][i] == "-tree" && i + 1 < sConfFile[index].size() ) {
      out_command["tree"] = sConfFile[index][i + 1];
      i++;
      // if (out_command[4].empty()){
      //   out_command[4] = sConfFile[index][i+1];
      //   i = i+1;
      // } else {
      //   cout << "Error: Multiple trees in Page: " << page << "--" << GetPageTitle(page).Data() << "\t coomand: " << nCommand << endl;
      //   exit(1);
      // }
    } else if( sConfFile[index][i] == "-grid" ) {
      out_command["grid"] = "grid";
      // if (out_command[5].empty()){ // grid option only works with TreeDraw
      //   out_command[5] = "grid";
      // } else {
      //   cout << "Error: Multiple setup of grid in Page: " << page << "--" << GetPageTitle(page).Data() << "\t coomand: " << nCommand << endl;
      //   exit(1);
      // }
    } else if( sConfFile[index][i] == "-logx" ) {
      out_command["logx"] = "logx";
    } else if( sConfFile[index][i] == "-logy" ) {
      out_command["logy"] = "logy";
    } else if( sConfFile[index][i] == "-logz" ) {
      out_command["logz"] = "logz";
    } else if( sConfFile[index][i] == "-nostat" ) {
      out_command["nostat"] = "nostat";
    } else if( sConfFile[index][i] == "-noshowgolden" ) {
      out_command["noshowgolden"] = "noshowgolden";
    } else {  // every thing else is regarded as cut
      out_command["cut"] = sConfFile[index][i];
      // if (out_command[1].empty()) {
      //   out_command[1] = sConfFile[index][i];
      // } else {
      //   cout << "Error: Multiple cut conditions in Page: " << page << "--" << GetPageTitle(page).Data() << "\t coomand: " << nCommand << endl;
      //   exit(1);
      // }
    }
  }

  if( fVerbosity >= 1 ) {
    cout << sConfFile[index].size() << ": ";
    for(UInt_t i=0; i<sConfFile[index].size(); i++) {
      cout << sConfFile[index][i] << " ";
    }
    cout << endl;
    for(UInt_t i=0; i<out_command.size(); i++) {
      cout << i << ": " << out_command[i] << endl;
    }
  }

  //return out_command;
  return;
}

void OnlineConfig::OverrideRootFile( int runnumber )
{
  // Override the ROOT file defined in the cfg file If
  // protorootfile is used, construct filename using it, otherwise
  // uses a helper macro "GetRootFileName.C(uint_t runnumber)
  cout << "Root file defined before was: " << rootfilename << endl;
  if( !protorootfile.empty() ) {
    //char runnostr[10];
    //sprintf(runnostr,"%04i",runnumber);

    ostringstream runnostr;
    runnostr << runnumber;
    protorootfile = ReplaceAll(protorootfile, "XXXXX", runnostr.str());
    rootfilename = protorootfile;
    // string temp = rootfilename(rootfilename.Last('_')+1,rootfilename.Length());
    // fRunNumber = atoi(temp(0,temp.Last('.')).Data());

    fRunNumber = runnumber;
    cout << "Protorootfile set, use it: " << rootfilename << endl;
  } else {
    string fnmRoot = "/adaq1/data1/sbs";
    if( getenv("ROOTFILES") )
      fnmRoot = getenv("ROOTFILES");
    else
      cout << "ROOTFILES env variable was not found going with default: " << fnmRoot << endl;

    cout << " Looking for file with runnumber " << runnumber << " in " << fnmRoot << endl;

    DIR* dirSearch;
    struct dirent* entSearch;
    const string daqConfigs[3] = {"e1209019_trigtest", "gmn", "bbgem_replayed"};
    bool found = false;
    if( (dirSearch = opendir(fnmRoot.c_str())) != nullptr ) {
      while( !found && (entSearch = readdir(dirSearch)) != nullptr ) {
        string fullname = entSearch->d_name;
        ostringstream partialname;
        for( const auto& daqConfig: daqConfigs ) {
          partialname << daqConfig << "_" << runnumber << ".root";
          if( fullname.find(partialname.str()) != string::npos ) {
            found = true;
          } else {
            partialname.clear();
            if( fMonitor ) {
              partialname << daqConfig << "_" << runnumber << ".adaq1";
              found = fullname.find(partialname.str()) != string::npos;
            } else {
              partialname << daqConfig << "_" << runnumber << ".0.root";
              if( fVerbosity >= 1 )
                cout << __PRETTY_FUNCTION__ << "\t" << __LINE__ << endl
                     << "Looking for a segmented output. Looking at segment 000 only" << endl;
              found = fullname.find(partialname.str()) != string::npos;
            }
          }
          if( found ) {
            rootfilename += fnmRoot;
            rootfilename += "/";
            rootfilename += fullname;
            break;
          }
        }
      }
      closedir(dirSearch);
    }

    if( found ) {
      cout << "\t found file " << rootfilename << endl;
      fRunNumber = runnumber;
    } else {
      cout << "double check your configurations and files. Quitting" << endl;
      exit(1);
    }
  }
}
