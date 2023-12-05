#ifndef panguinOnlineConfig_h
#define panguinOnlineConfig_h

#include <utility>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <functional> // std::function

std::string DirnameStr( std::string path );
std::string BasenameStr( std::string path );

using uint_t = unsigned int;
using strstr_t = std::pair<std::string, std::string>;
using VecStr_t = std::vector<std::string>;
using ConfLines_t = std::vector<VecStr_t>;
using PageInfo_t = std::vector<std::pair<uint_t, uint_t>>;

std::string ReplaceAll(
  std::string str, const std::string& ostr, const std::string& nstr );
bool EndsWith( const std::string& str, const std::string& tail );

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
  std::string fImagesDir;         // Where to save individual images
  std::string plotsdir;           // Where to save plots
  // the config file, in memory
  ConfLines_t sConfFile;
  VecStr_t    fProtoRootFiles; // Candidate ROOT file names
  // pageInfo is the vector of the pages containing the sConfFile index
  //   and how many commands issued within that page (title, 1d, etc.)
  PageInfo_t  pageInfo;
  std::vector<strstr_t> cutList;
  std::vector<uint_t> GetDrawIndex( uint_t );
  bool fFoundCfg;
  bool fMonitor;
  int fVerbosity;
  int hist2D_nBinsX, hist2D_nBinsY;
  int fRunNumber;
  int fRunNoWidth;
  int fPageNoWidth;
  int fPadNoWidth;
  bool fPrintOnly;
  bool fSaveImages;

  int LoadFile( std::ifstream& infile, const std::string& filename );
  int CheckLoadIncludeFile( const std::string& sline,
                            const VecStr_t& strvect );

  struct CommandDef {
    CommandDef( std::string cmd, size_t nargs,
                std::function<void(const VecStr_t&)> action )
      : cmd_{std::move(cmd)}, nargs_{nargs}, action_{std::move(action)} {}
    std::string cmd_{};
    size_t nargs_{0};
    std::function<void(const VecStr_t&)> action_{nullptr};
  };
  static int ParseCommands( ConfLines_t::const_iterator pos,
                            ConfLines_t::const_iterator end,
                            const std::vector<CommandDef>& items );


public:
  struct CmdLineOpts {
    explicit CmdLineOpts(std::string f)
      : cfgfile(std::move(f))
    {}
    CmdLineOpts( std::string f, std::string d, std::string rf,
                 std::string gf, std::string rd, std::string pf,
                 std::string ifm, std::string pd, std::string id,
                 int rn, int v, bool po, bool si )
      : cfgfile(std::move(f))
      , cfgdir(std::move(d))
      , rootfile(std::move(rf))
      , goldenfile(std::move(gf))
      , rootdir(std::move(rd))
      , plotfmt(std::move(pf))
      , imgfmt(std::move(ifm))
      , plotsdir(std::move(pd))
      , imgdir(std::move(id))
      , run(rn)
      , verbosity(v)
      , printonly(po)
      , saveimages(si)
    {}
    std::string cfgfile;
    std::string cfgdir;
    std::string rootfile;
    std::string goldenfile;
    std::string rootdir;
    std::string plotfmt;
    std::string imgfmt;
    std::string plotsdir;
    std::string imgdir;
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
  std::string SubstituteRunNumber( std::string str, int runnumber ) const;

  const std::string& GetConfFilePath() const { return fConfFilePath; }
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
  const std::string& GetImagesDir() const { return fImagesDir; };
  int GetVerbosity() const { return fVerbosity; }
  int GetRunNoWidth() const { return fRunNoWidth; }
  int GetPageNoWidth() const { return fPageNoWidth; }
  int GetPadNoWidth() const { return fPadNoWidth; }
  bool DoPrintOnly() const { return fPrintOnly; }
  bool DoSaveImages() const { return fSaveImages; }
  const std::string& GetDefinedCut( const std::string& ident );
  VecStr_t GetCutIdent();
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
