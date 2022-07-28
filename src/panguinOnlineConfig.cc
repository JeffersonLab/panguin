#include "panguinOnlineConfig.hh"
#include <string>
#include <iostream>
#include <sstream>
#include <utility>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <iomanip>    // quoted, setw, setfill
#include <cctype>     // isalnum
#include <algorithm>  // find_if
#include <regex>

using namespace std;

#define ALL(c) (c).begin(), (c).end()

//_____________________________________________________________________________
// Replace all occurrences in 'str' of 'ostr' with 'nstr'
string ReplaceAll( string str, const string& ostr, const string& nstr )
{
  size_t pos = 0, ol = ostr.size(), nl = nstr.size();
  while( true ) {
    pos = str.find(ostr, pos);
    if( pos == string::npos )
      break;
    str.replace(pos, ol, nstr);
    pos += nl;
  }
  return str;
}

//_____________________________________________________________________________
// Check if given string 'str' ends with 'tail'
bool EndsWith( const string& str, const string& tail )
{
  auto sl = str.length(), tl = tail.length();
  return sl >= tl && str.substr(sl - tl, tl) == tail;
}

//_____________________________________________________________________________
// Get directory name part of 'path'
string DirnameStr( string path )
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

//_____________________________________________________________________________
// Get directory name part of 'path'
string BasenameStr( string path )
{
  auto pos = path.rfind('/');
  if( pos != string::npos )
    path.erase(0, pos+1);
  return path;
}

//_____________________________________________________________________________
// Try to open 'filename'. If 'filename' is a relative path (does not start
// with '/'), try opening it in the current directory and, if not found, in
// any of the directories given in 'path'.
// Returns a filestream and path string where the file was found. Test the
// filestream to determine whether the file was successfully opened.
static pair<ifstream, string>
OpenInPath( const string& filename, const string& path )
{
  string dirname = DirnameStr(filename);
  string foundpath;
  foundpath.reserve(path.length() + dirname.length() + 1);
  ifstream infile(filename);
  if( !infile ) {
    if( !filename.empty() && filename[0] != '/' ) {
      string trypath;
      trypath.reserve(path.length() + filename.length() + 1);
      istringstream istr(path);
      while( getline(istr, trypath, ':') ) {
        if( trypath.empty() )
          continue;
        foundpath = trypath;
        trypath += "/" + filename;
        infile.clear();
        infile.open(trypath);
        if( infile ) {
          if( dirname != "." )
            foundpath += "/" + dirname;
          break;
        }
      }
    }
  } else {
    foundpath = dirname;
  }
  return make_pair(std::move(infile), std::move(foundpath));
}

//_____________________________________________________________________________
// Append 'dir' to 'path'
static void AppendToPath( string& path, const string& dir )
{
  if( dir.empty() )
    return;
  if( !path.empty() )
    path += ":";
  path += dir;
}

//_____________________________________________________________________________
// Expand specials and environment variables
static string ExpandFileName( string str )
{
  if( str.empty() )
    return str;
  if( str[0] == '~' ) {
    auto* home = getenv("HOME");
    if( home )
      str.replace(0, 1, home);
  }
  if( str.size() < 2 )
    return str;
  size_t pos;
  while( (pos = str.find('$')) != string::npos ) {
    auto iend = find_if(str.begin() + pos + 1, str.end(), []( int c ) {
      return (!isalnum(c) && c != '_');
    });
    auto len = iend - str.begin() - pos - 1;
    auto envvar = str.substr(pos + 1, len);
    auto* envval = getenv(envvar.c_str());
    if( envval )
      str.replace(pos, len + 1, envval);
  }
  return str;
}

//_____________________________________________________________________________
// Change file extension of 'path' to 'ext', or add 'ext' if none exists
static void ChangeExtension( string& path, const string& ext )
{
  auto pos = path.rfind('.');
  if( pos == string::npos )
    path += "." + ext;
  else
    path.replace(pos + 1, path.length() - pos - 1, ext);
}

//_____________________________________________________________________________
// Check if 'var' is non-empty. If so, print message and return true.
static bool IsSet( const string& var, const string& name )
{
  if( var.empty() )
    return false;
  cerr << "Warning: Command line option for " << name << " overrides value in "
       << "configuration file." << endl;
  return true;
}

//_____________________________________________________________________________
static bool PrependDir( const string& dir, string& path,
                        const string& name1, const string& name2 )
{
  if( path.empty() )
    return false;
  if( path[0] == '/' ) {
    cerr << "Warning: ignoring " << name1 << " because " << name2 << " has "
         << "an absolute path" << endl;
    return false;
  }
  path = dir + "/" + path;
  return true;
}

//_____________________________________________________________________________
static void OverrideFormat( string& proto, const string& fmt,
                            const string& name1, const string& name2 )
{
  if( !EndsWith(proto, fmt) &&
      !(EndsWith(proto, "%E") ||
        EndsWith(proto, "%F")) ) {
    cerr << "Warning: overriding " << name1 << " of " << name2 << " with "
         << fmt << endl;
    ChangeExtension( proto, fmt );
  }
}

//_____________________________________________________________________________
// Attempt to extract the run number from a ROOT file name.
// Since there is no agreed-upon filename pattern, use a simple heuristic
// that the run number is 4 or 5 digits between an underscore and either
// another underscore or a period: _12345_ or _1234.
static int ExtractRunNumber( const string& filename )
{
  regex re("_([0-9]{4,5})[\\._]");
  smatch sm;
  if( regex_search(filename, sm, re) && sm.size() > 1)
    return stoi(sm[1].str());
  else
    return 0;
}

//_____________________________________________________________________________
// Constructor.  Without an argument, use default config
OnlineConfig::OnlineConfig()
  : OnlineConfig("default.cfg") {}

//_____________________________________________________________________________
// Constructor.  Takes the config file name as the only argument.
// Loads up the configuration file, and stores its contents for access.
OnlineConfig::OnlineConfig( const string& config_file_name )
  : OnlineConfig(CmdLineOpts{config_file_name}) {}

//_____________________________________________________________________________
OnlineConfig::OnlineConfig( const CmdLineOpts& opts )
  : confFileName(opts.cfgfile)
  , fPlotFormat(opts.plotfmt)
  , fImageFormat(opts.imgfmt)
  , fImagesDir(opts.imgdir)
  , plotsdir(opts.plotsdir)
  , fFoundCfg(false)
  , fMonitor(false)
  , fVerbosity(opts.verbosity)
  , hist2D_nBinsX(0)
  , hist2D_nBinsY(0)
  , fRunNumber(opts.run)
  , fRunNoWidth(5)
  , fPageNoWidth(2)
  , fPadNoWidth(2)
  , fPrintOnly(opts.printonly)
  , fSaveImages(opts.saveimages)
{
  // Pick up config file directory/path form environment.
  // A config dir or path given on the command line takes preference.
  string cfgpath = opts.cfgdir;
  const char* env_cfgdir = getenv("PANGUIN_CONFIG_PATH");
  if( env_cfgdir )
    AppendToPath(cfgpath, env_cfgdir);
  if( fVerbosity > 0 )
    cout << "config file path = " << cfgpath << endl;

  ifstream infile;
  std::tie(infile, fConfFileDir) = OpenInPath(confFileName, cfgpath);

  if( !infile ) {
    cerr << "OnlineConfig() ERROR: cannot find " << confFileName << endl;
    cerr << "You need a configuration to run.  Ask an expert." << endl;
    fFoundCfg = false;
    return;
  }
  fFoundCfg = true;
  confFileName = BasenameStr(confFileName);

  // Ensure that fConfFilePath contains any relative path from confFileName
  if( fConfFileDir != "." && cfgpath.find(fConfFileDir) == string::npos )
    cfgpath = cfgpath.empty() ? fConfFileDir : fConfFileDir + ":" + cfgpath;
  fConfFilePath = std::move(cfgpath);

  string fullpath = fConfFileDir + "/" + confFileName;

  cout << "GUI Configuration loading from " << fullpath << endl;
  try {
    auto ret = LoadFile(infile, fullpath);
    if( ret != 0 )
      throw std::runtime_error(
        "Error loading configuration file \"" + fullpath + "\"");
  } catch( const std::runtime_error& e ) {
    cerr << e.what() << endl;
    fFoundCfg = false;
  }
}

//_____________________________________________________________________________
int OnlineConfig::CheckLoadIncludeFile(
  const string& sline, const std::vector<std::string>& strvect )
{
  if( strvect[0] == "include" ) {
    if( strvect.size() != 2 || strvect[1].empty() ) {
      cerr << "Too " << (strvect.size() == 1 ? "few" : "many")
           << "arguments for include statement "
           << "(expect 1 = file name). Skipping line: " << endl
           << "--> " << std::quoted(sline) << endl;
      return 0;
    }
    string fname = ExpandFileName(strvect[1]), incdir;
    ifstream ifs;
    std::tie(ifs, incdir) = OpenInPath(fname, fConfFilePath);
    if( !ifs )
      throw std::runtime_error("Error opening include file \"" + fname + "\"");
    if( fVerbosity >= 1 )
      cout << "Loading include file " << std::quoted(fname) << endl;
    auto ret = LoadFile(ifs, fname);
    if( ret != 0 )
      throw std::runtime_error("Error loading include file \"" + fname + "\"");
    return 1;
  }
  return 0;
}

//_____________________________________________________________________________
// Reads in the Config File, and makes the proper calls to put
//  the information contained into memory.
int OnlineConfig::LoadFile( std::ifstream& infile, const string& filename )
{
  if( !infile )
    return 1;

  const char comment = '#';
  vector<string> strvect;
  string sinput, sline;
  while( getline(infile, sline) ) {
    if( sline.empty() || sline.find(comment) != string::npos ) continue;
    istringstream istr(sline);
    string field;
    strvect.clear();
    while( istr >> field )
      strvect.push_back(std::move(field));
    if( CheckLoadIncludeFile(sline, strvect) )
      continue;
    sConfFile.push_back(std::move(strvect));
  }

  if( fVerbosity >= 1 ) {
    cout << "OnlineConfig::LoadFile()\n";
    for( uint_t ii = 0; ii < sConfFile.size(); ii++ ) {
      cout << "Line " << ii << endl << "  ";
      for( const auto& field: sConfFile[ii] )
        cout << field << " ";
      cout << endl;
    }
  }

  cout << setw(6) << sConfFile.size() << " lines read from "
       << filename << endl;

  return 0;
}

//_____________________________________________________________________________
string OnlineConfig::SubstituteRunNumber( string str, int runnumber ) const
{
  ostringstream ostr;
  if( fRunNoWidth > 0 )
    ostr << setw( fRunNoWidth ) << setfill('0');
  ostr << runnumber;
  str = ReplaceAll(str, "XXXXX", ostr.str());
  return ReplaceAll(str, "%R", ostr.str());
}

//_____________________________________________________________________________
static ConfLines_t::const_iterator
ParsePageInfo( ConfLines_t::const_iterator pos, ConfLines_t::const_iterator end,
               PageInfo_t& pageInfo )
{
  auto beg = pos, start = pos, first_page = end;
  bool counting = false;
  uint_t command_cnt = 0;
  for( ; pos != end; ++pos ) {
    if( (*pos)[0] == "newpage" ) {
      if( counting )
        goto finish_page;
      counting = true;
      start = first_page = pos;
    } else if( counting ) {
      ++command_cnt;
      if( pos + 1 == end ) {
  finish_page:
        pageInfo.emplace_back(start - beg, command_cnt);
        command_cnt = 0;
        start = pos;
      }
    }
  }
  return first_page;
}

//_____________________________________________________________________________
int OnlineConfig::ParseCommands( ConfLines_t::const_iterator pos,
                                 ConfLines_t::const_iterator end,
                                 const vector<CommandDef>& items )
{
  for( ; pos != end; ++pos ) {
    const auto& line = *pos;
    for( const auto& item: items ) {
      if( item.cmd != line[0] )
        continue;
      auto narg = line.size()-1;
      if( narg != item.narg ) {
        if( narg < item.narg ) {
          cerr << "ERROR: not enough arguments for " << item.cmd << " command,"
               << "needs " << item.narg << ", found " << narg
               << ". Command skipped." << endl;
          continue;
        } else
          cerr << "WARNING: too many arguments for " << item.cmd << " command,"
               << "expect " << item.narg << ", found " << narg
               << ", ignoring extras" << endl;
      }
      if( item.action )
        item.action(line);
      else
        cerr << "WARNING: no action for " << item.cmd << " command? "
             << "Call expert." << endl;
      break;
    }
  }
  return 0;
}

//_____________________________________________________________________________
//  Goes through each line of the config [must have been LoadFile()'d]
//   and interprets.
bool OnlineConfig::ParseConfig()
{
  if( !fFoundCfg )
    return false;

  // Find "newpage" commands and store their locations and lengths
  auto first_page = ParsePageInfo(ALL(sConfFile), pageInfo);

  // List of defined commands and corresponding actions
  vector<CommandDef> cmddefs = {
    {"watchfile",
      0, [&]( const VecStr_t& ) {
      fMonitor = true;
    }},
    {"2DbinsX",
      0, [&]( const VecStr_t& line ) {
      hist2D_nBinsX = stoi(line[1]);
    }},
    {"2DbinsY",
      0, [&]( const VecStr_t& line ) {
      hist2D_nBinsY = stoi(line[1]);
    }},
    {"definecut",
      2, [&]( const VecStr_t& line ) {
      cutList.emplace_back(line[1], line[2]);
    }},
    {"rootfile",
      1, [&]( const VecStr_t& line ) {
      if( !IsSet(rootfilename, "rootfile") )
        rootfilename = ExpandFileName(line[1]);
      fRunNumber = ExtractRunNumber(rootfilename);
    }},
    {"goldenrootfile",
      1, [&]( const VecStr_t& line ) {
      goldenrootfilename = ExpandFileName(line[1]);
    }},
    {"protorootfile",
      1, [&]( const VecStr_t& line ) {
      fProtoRootFiles.push_back(ExpandFileName(line[1]));
    }},
    {"guicolor",
      1, [&]( const VecStr_t& line ) {
      guicolor = line[1];
    }},
    {"plotsDir",
      1, [&]( const VecStr_t& line ) {
      if( !IsSet(plotsdir, "plotsDir") )
        plotsdir = ExpandFileName(line[1]);
    }},
    {"imagesDir",
      1, [&]( const VecStr_t& line ) {
      if( !IsSet(fImagesDir, "imagessDir") )
        fImagesDir = ExpandFileName(line[1]);
    }},
    {"plotFormat",
      1, [&]( const VecStr_t& line ) {
      if( !IsSet(fPlotFormat, "plotFormat") )
        fPlotFormat = line[1];
    }},
    {"imageFormat",
      1, [&]( const VecStr_t& line ) {
      if( !IsSet(fImageFormat, "imageFormat") )
        fImageFormat = line[1];
    }},
    {"rootfilespath",
      1, [&]( const VecStr_t& line ) {
      fRootFilesPath = ExpandFileName(line[1]);
    }},
    {"protoplotfile",
      1, [&]( const VecStr_t& line ) {
      fProtoPlotFile = ExpandFileName(line[1]);
    }},
    {"protoplotpagefile",
      1, [&]( const VecStr_t& line ) {
      fProtoPlotPageFile = ExpandFileName(line[1]);
    }},
    {"protoimagefile",
      1, [&]( const VecStr_t& line ) {
      fProtoImageFile = ExpandFileName(line[1]);
    }},
    {"protomacroimagefile",
      1, [&]( const VecStr_t& line ) {
      fProtoMacroImageFile = ExpandFileName(line[1]);
    }}
  };

  ParseCommands(sConfFile.begin(), first_page, cmddefs);

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

  // Set fallback defaults
  if( fPlotFormat.empty() )
    fPlotFormat = "pdf";
  if( fImageFormat.empty() )
    fImageFormat = "png";
  if( fProtoPlotFile.empty() )
    fProtoPlotFile = "summaryPlots_%R_%C.%E";
  if( fProtoPlotPageFile.empty() )
    fProtoPlotPageFile = "summaryPlots_%R_page%P_%C.%E";
  if( fProtoImageFile.empty() )
    fProtoImageFile = "hydra_%R_%V_%C.%F";
  if( fProtoMacroImageFile.empty() )
    fProtoMacroImageFile = "hydra_%R_page%P_pad%D_%C.%F";

  // Prepend output directory to plot file prototypes
  if( !plotsdir.empty() ) {
    bool needit =
      PrependDir(plotsdir, fProtoPlotFile, "plotsdir", "protoplotfile") ||
      PrependDir(plotsdir, fProtoPlotPageFile, "plotsdir", "protoplotpagefile");
    if( !needit )
      plotsdir.clear();  // Don't create possibly spurious directory
  }
  // Prepend output directory to image file prototypes
  if( !fImagesDir.empty() ) {
    bool needit =
      PrependDir(fImagesDir, fProtoImageFile, "imagesdir", "protoimagefile") ||
      PrependDir(fImagesDir, fProtoMacroImageFile, "imagesdir", "protomacroimagefile");
    if( !needit )
      fImagesDir.clear();  // Don't create possibly spurious directory
  }

  // Honor requested plot format
  OverrideFormat(fProtoPlotFile, fPlotFormat, "plotFormat", "protoplotfile");
  OverrideFormat(fProtoPlotPageFile, fPlotFormat, "plotFormat", "protoplotpagefile");
  // Honor requested image format
  OverrideFormat(fProtoImageFile, fImageFormat, "imageFormat", "protoimagefile");
  OverrideFormat(fProtoMacroImageFile, fImageFormat, "imageFormat", "protomacroimagefile");

  return true;
}

//_____________________________________________________________________________
// Returns the defined cut, according to the identifier
const string& OnlineConfig::GetDefinedCut( const string& ident )
{
  static const string nullstr{};

  for( const auto& cut: cutList ) {
    if( cut.first == ident ) {
      return cut.second;
    }
  }
  return nullstr;
}

//_____________________________________________________________________________
// Returns a vector of the cut identifiers, specified in config
vector<string> OnlineConfig::GetCutIdent()
{
  vector<string> out;

  for( const auto& cut: cutList ) {
    out.push_back(cut.first);
  }
  return out;
}

//_____________________________________________________________________________
// Check if last word on line is "logy"
bool OnlineConfig::IsLogy( uint_t page )
{
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
    for( const auto& opts: sConfFile[page_index] ) {
      cout << opts << " ";
    }
  }
  return false;
}

//_____________________________________________________________________________
// If defined in the config, will return those dimensions
//  for the indicated page.  Otherwise, will return the
//  calculated dimensions required to fit all histograms.
pair<uint_t, uint_t> OnlineConfig::GetPageDim( uint_t page )
{
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

//_____________________________________________________________________________
// Returns the title of the page.
//  if it is not defined in the config, then return "Page #"
string OnlineConfig::GetPageTitle( uint_t page )
{
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

//_____________________________________________________________________________
// Returns an index of where to find the draw commands within a page
//  within the sConfFile vector
vector<uint_t> OnlineConfig::GetDrawIndex( uint_t page )
{
  vector<uint_t> index;
  uint_t iter_command = pageInfo[page].first + 1;

  for( uint_t i = 0; i < pageInfo[page].second; i++ ) {
    if( sConfFile[iter_command + i][0] != "title" ) {
      index.push_back(iter_command + i);
    }
  }

  return index;
}

//_____________________________________________________________________________
// Returns the number of histograms that have been requested for this page
uint_t OnlineConfig::GetDrawCount( uint_t page )
{
  uint_t draw_count = 0;

  for( uint_t i = 0; i < pageInfo[page].second; i++ ) {
    if( sConfFile[pageInfo[page].first + i + 1][0] != "title" ) draw_count++;
  }

  return draw_count;
}

//_____________________________________________________________________________
// Returns the vector of strings pertaining to a specific page, and
//   draw command from the config.
// Return map<string,string> in out_command:
// Following options are implemented:
//  1. "-drawopt" --> set draw options for histograms and tree variables
//  2. "-title" --> set title, enclose in double quotes
//  3. "-tree" --> set tree name
//  4. "-grid" --> set "grid" option
//  5. "-logx, -logy, -logz" --> draw with log x,y,z axis
//  6. "-nostat" --> don't show stats box
//  7. "-noshowgolden" --> don't show "golden" histogram even if goldenrootfile is defined
//  8. any option not preceded by these indicators is assumed to be a cut or macro expression:
// what options do we want?
//  all options on one line. First argument assumed to be histogram or tree name (or "macro")
//
void OnlineConfig::GetDrawCommand(
  uint_t page, uint_t nCommand, std::map<string, string>& out_command )
{
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
          //  This case uses neither "i = j;" or "break;", because we want to
          //  be able to include all the words in the title. The title will
          //  end before the end of the line only if it is delimited by quotes.
          title += " " + word;
        }
      }
      if( i == sConfFile[index].size() && sConfFile[index][i - 1].back() != '\"' ) {
        // unmatched double quote
        cout << "Error, unmatched double quote, please check you config file. Quitting" << endl;
        exit(1);  // FIXME: exit?
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
    for( const auto& field: sConfFile[index] ) {
      cout << field << " ";
    }
    cout << endl;
    int i = 0;
    for( const auto& cmd: out_command ) {
      cout << i++ << ": [" << std::quoted(cmd.first)
           << ", " << std::quoted(cmd.second) << "]" << endl;
    }
  }
}

//_____________________________________________________________________________
// Override the ROOT file defined in the cfg file. This is called when the
// user specifies a run number on the command line.
void OnlineConfig::OverrideRootFile( int runnumber )
{
  if( !rootfilename.empty() )
    cout << "Root file defined before was: " << rootfilename << endl;

  string fnmRootPath;
  AppendToPath(fnmRootPath, fRootFilesPath);
  auto* envar = getenv("ROOTFILES");
  if( envar )
    AppendToPath(fnmRootPath, envar);
  AppendToPath(fnmRootPath, "rootfiles");

  auto protofiles = fProtoRootFiles;
  if( protofiles.empty() )
    protofiles.push_back(rootfilename);
  bool found = false;
  for( auto& protofile: protofiles ) {
    // try opening protofile in path
    assert(!protofile.empty());  // else error in ParseConfig
    protofile = SubstituteRunNumber(protofile, runnumber);
    cout << " Looking for ROOT file with runnumber " << runnumber
         << " in " << fnmRootPath << endl;
    ifstream ifs;
    string fp;
    std::tie(ifs, fp) = OpenInPath(protofile, fnmRootPath);
    if( ifs ) {
      rootfilename = fp + "/" + BasenameStr(protofile);
      found = true;
      break;
    }
  }

  if( found ) {
    cout << "\t found file " << rootfilename << endl;
  } else {
    cout << "No ROOT file found. Double check your configurations and files. "
         << "Quitting ..." << endl;
    exit(1);
  }

  fRunNumber = runnumber;
}

//_____________________________________________________________________________
