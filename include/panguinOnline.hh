///////////////////////////////////////////////////////////////////
//  Macro to help with online analysis
//    B. Moffit  Oct. 2003
#ifndef panguinOnline_h
#define panguinOnline_h 1

#include <TTree.h>
#include <TFile.h>
#include <TGButton.h>
#include <TGFrame.h>
#include <TGListBox.h>
#include <TRootEmbeddedCanvas.h>
#include "TGLabel.h"
#include "TGString.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <TString.h>
#include <TCut.h>
#include <TTimer.h>
#include "TH1.h"
#include "TH2.h"
#include "TH3.h"
#include "panguinOnlineConfig.hh"

#define UPDATETIME 10000

class OnlineGUI {
  TGMainFrame* fMain = nullptr;
  TGHorizontalFrame* fTopframe = nullptr;
  TGVerticalFrame* vframe = nullptr;
  TGListBox* fPageListBox = nullptr;
  TGPictureButton* wile = nullptr;
  TGTextButton* fNow = nullptr; // current time
  TGTextButton* fLastUpdated = nullptr; // plots last updated
  TGTextButton* fRootFileLastUpdated = nullptr; // Root file last updated
  TRootEmbeddedCanvas* fEcanvas = nullptr;
  TGHorizontalFrame* fBottomFrame = nullptr;
  TGHorizontalFrame* hframe = nullptr;
  TGTextButton* fNext = nullptr;
  TGTextButton* fPrev = nullptr;
  TGTextButton* fExit = nullptr;
  TGLabel* fRunNumber = nullptr;
  TGTextButton* fPrint = nullptr;
  TCanvas* fCanvas = nullptr; // Present Embedded canvas
  OnlineConfig fConfig;
  UInt_t current_page;
  TFile* fRootFile = nullptr;
  TFile* fGoldenFile = nullptr;
  Bool_t doGolden;
  std::vector<TTree*> fRootTree;
  std::vector<Int_t> fTreeEntries;
  std::vector<std::pair<TString, TString> > fileObjects;
  std::vector<std::vector<TString> > treeVars;
  UInt_t runNumber;
  TTimer* timer = nullptr;
  TTimer* timerNow = nullptr; // used to update time
  Bool_t fUpdate;
  Bool_t fFileAlive;
  Bool_t fPrintOnly;
  Bool_t fSaveImages;
  TH1D* mytemp1d = nullptr;
  TH2D* mytemp2d = nullptr;
  TH3D* mytemp3d = nullptr;
  TH1D* mytemp1d_golden = nullptr;
  TH2D* mytemp2d_golden = nullptr;
  TH3D* mytemp3d_golden = nullptr;

  int fVerbosity;

public:
  explicit OnlineGUI( const OnlineConfig& config );
  explicit OnlineGUI( OnlineConfig&& config );
  void CreateGUI( const TGWindow* p, UInt_t w, UInt_t h );
  virtual ~OnlineGUI();
  void DoDraw();
  void DrawPrev();
  void DrawNext();
  void DoListBox( Int_t id );
  void CheckPageButtons();
  // Specific Draw Methods
  Bool_t IsHistogram( const TString& );
  void GetFileObjects();
  void GetTreeVars();
  void GetRootTree();
  UInt_t GetTreeIndex( TString );
  UInt_t GetTreeIndexFromName( const TString& );
  void TreeDraw( const std::map<std::string, std::string>& command );
  void HistDraw( const std::map<std::string, std::string>& command );
  void MacroDraw( const std::map<std::string, std::string>& command );
  void LoadDraw( const std::map<std::string, std::string>& command );
  void LoadLib( const std::map<std::string, std::string>& command );
  void SaveImage( TObject* o, const std::map<std::string, std::string>& command ) const;
  void DoDrawClear();
  void TimerUpdate();
  void UpdateCurrentTime();  // update current time
  static void BadDraw( const TString& );
  void CheckRootFile();
  Int_t OpenRootFile();
  void PrintToFile();
  void PrintPages();
  void MyCloseWindow();
  void CloseGUI();
  void SetVerbosity( int ver ) { fVerbosity = ver; }
  ClassDef(OnlineGUI, 0)
};

#endif //panguinOnline_h
