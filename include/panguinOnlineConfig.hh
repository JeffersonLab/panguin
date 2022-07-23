#ifndef panguinOnlineConfig_h
#define panguinOnlineConfig_h

#include <utility>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <string>

using uint_t = unsigned int;
using strstr_t = std::pair<std::string, std::string>;

class OnlineConfig {
  // Class that takes care of the config file
  std::string confFileName;       // config filename
  std::string fConfFilePath;      // Search path for configuration files
  std::string rootfilename;       //  Just the name
  std::string goldenrootfilename; // Golden rootfile for comparisons
  std::string protorootfile;      // Prototype for getting the rootfilename
  std::string guicolor;           // User's choice of background color
  std::string fPlotFilePrefix;    // File name prefix for saved plots (default: summaryPlots)
  std::string fPlotFormat;        // File format for saved plots (default: pdf)
  std::string fImageFilePrefix;   // File name prefix for saved image files (default: hydra)
  std::string fImageFormat;       // File format for saved image files (default: png)
  std::string plotsdir;           // Where to save plots
  // the config file, in memory
  std::vector<std::vector<std::string>> sConfFile;
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


  int LoadFile( std::ifstream& infile );

public:
  struct CmdLineOpts {
    std::string cfgfile;
    std::string cfgdir;
    std::string plotpfx;
    std::string plotfmt;
    std::string imgpfx;
    std::string imgfmt;
    std::string outdir;
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

  const std::string& GetGuiDirectory() const { return fConfFilePath; }
  const std::string& GetConfFileName() const { return confFileName; }
  void Get2DnumberBins( int& nX, int& nY ) const
  {
    nX = hist2D_nBinsX;
    nY = hist2D_nBinsY;
  }
  void SetVerbosity( int ver ) { fVerbosity = ver; }
  const char *GetRootFile() const { return rootfilename.c_str(); };
  const std::string& GetGoldenFile() const { return goldenrootfilename; };
  const std::string& GetGuiColor() const { return guicolor; };
  const std::string& GetPlotFilePrefix() const { return fPlotFilePrefix; }
  const std::string& GetPlotFormat() const { return fPlotFormat; }
  const std::string& GetImageFilePrefix() const { return fImageFilePrefix; }
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
