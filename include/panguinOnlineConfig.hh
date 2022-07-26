#ifndef panguinOnlineConfig_h
#define panguinOnlineConfig_h

#include <utility>
#include <fstream>
#include <vector>
#include <map>
#include <string>

std::string DirnameStr( std::string path );
std::string BasenameStr( std::string path );

using uint_t = unsigned int;
using strstr_t = std::pair<std::string, std::string>;

std::string ReplaceAll(
  std::string str, const std::string& ostr, const std::string& nstr );
std::string SubstituteRunNumber( std::string str, int runnumber );

class OnlineConfig {
  // Class that takes care of the config file
  std::string confFileName;       // config filename
  std::string fConfFileDir;       // Directory where config file found
  std::string fConfFilePath;      // Search path for configuration files
  std::string fRootFilesPath;     // Search path for ROOT files
  std::string rootfilename;       //  Just the name
  std::string goldenrootfilename; // Golden rootfile for comparisons
  std::string guicolor;           // User's choice of background color
  std::string fProtoPlotFile;
  std::string fProtoPlotPageFile;
  std::string fProtoImageFile;
  std::string fProtoMacroImageFile;
  std::string fPlotFormat;        // File format for saved plots (default: pdf)
  std::string fImageFormat;       // File format for saved image files (default: png)
  std::string plotsdir;           // Where to save plots
  // the config file, in memory
  std::vector<std::vector<std::string>> sConfFile;
  std::vector<std::string> fProtoRootFiles; // Candidate ROOT file names
  // pageInfo is the vector of the pages containing the sConfFile index
  //   and how many commands issued within that page (title, 1d, etc.)
  std::vector<std::pair<uint_t, uint_t> > pageInfo;
  std::vector<strstr_t> cutList;
  std::vector<uint_t> GetDrawIndex( uint_t );
  bool fFoundCfg;
  bool fMonitor;
  int fVerbosity;
  int hist2D_nBinsX, hist2D_nBinsY;
  int fRunNumber;
  bool fPrintOnly;
  bool fSaveImages;

  int LoadFile( std::ifstream& infile, const std::string& filename );
  int CheckLoadIncludeFile( const std::string& sline,
                            const std::vector<std::string>& strvect );
//  bool MatchFilename( const std::string& fullname,
//                      const std::string& daqConfig, int runnumber ) const;

public:
  struct CmdLineOpts {
    std::string cfgfile;
    std::string cfgdir;
    std::string plotfmt;
    std::string imgfmt;
    std::string plotsdir;
    int run{0};
    int verbosity{0};
    bool printonly{false};
    bool saveimages{false};
  };

  OnlineConfig();
  explicit OnlineConfig( const std::string& config_file_name );
  explicit OnlineConfig( const CmdLineOpts& opts );
  bool ParseConfig();
  int GetRunNumber() const { return fRunNumber; }

  const std::string& GetGuiDirectory() const { return fConfFileDir; }
  const std::string& GetConfFileName() const { return confFileName; }
  void Get2DnumberBins( int& nX, int& nY ) const
  {
    nX = hist2D_nBinsX;
    nY = hist2D_nBinsY;
  }
  void SetVerbosity( int ver ) { fVerbosity = ver; }
  const char* GetRootFile() const { return rootfilename.c_str(); };
  const char* GetGoldenFile() const { return goldenrootfilename.c_str(); };
  const std::string& GetGuiColor() const { return guicolor; };
  const std::string& GetProtoPlotFile() const { return fProtoPlotFile; }
  const std::string& GetProtoPlotPageFile() const { return fProtoPlotPageFile; }
  const std::string& GetProtoImageFile() const { return fProtoImageFile; }
  const std::string& GetProtoMacroImageFile() const { return fProtoMacroImageFile; }
  const std::string& GetPlotFormat() const { return fPlotFormat; }
  const std::string& GetImageFormat() const { return fImageFormat; }
  const std::string& GetPlotsDir() const { return plotsdir; };
  int GetVerbosity() const { return fVerbosity; }
  bool DoPrintOnly() const { return fPrintOnly; }
  bool DoSaveImages() const { return fSaveImages; }
  const std::string& GetDefinedCut( const std::string& ident );
  std::vector<std::string> GetCutIdent();
  // Page utilites
  uint_t GetPageCount() { return pageInfo.size(); };
  std::pair<uint_t, uint_t> GetPageDim( uint_t );
  bool IsLogy( uint_t page );
  std::string GetPageTitle( uint_t );
  uint_t GetDrawCount( uint_t );           // Number of histograms in a page
  void GetDrawCommand( uint_t, uint_t, std::map<std::string, std::string>& );
  void OverrideRootFile( int runnumber );
  bool IsMonitor() const { return fMonitor; };
};

#endif //panguinOnlineConfig_h
