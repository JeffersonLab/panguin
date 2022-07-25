///////////////////////////////////////////////////////////////////
//  Macro to help with online analysis
//    B. Moffit  Oct. 2003
///////////////////////////////////////////////////////////////////

#include "panguinOnline.hh"
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <TMath.h>
#include <TBranch.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TGImageMap.h>
#include <TGFileDialog.h>
#include <TKey.h>
#include <TSystem.h>
#include <TLatex.h>
#include <TText.h>
#include "TPaveText.h"
#include <TApplication.h>
#include "TEnv.h"
#include "TRegexp.h"
#include "TGraph.h"
#include "TGaxis.h"
#include <map>
#include <utility>

#define OLDTIMERUPDATE

ClassImp(OnlineGUI)

using namespace std;

///////////////////////////////////////////////////////////////////
//  Class: OnlineGUI
//
//    Creates a GUI to display the commands used in OnlineConfig
//
//

OnlineGUI::OnlineGUI(OnlineConfig&& config)
  : fConfig(std::move(config))
  , runNumber(0)
  , timer(nullptr)
  , timerNow(nullptr)
  , fFileAlive(kFALSE)
  , fVerbosity(fConfig.GetVerbosity())
  , fSaveImages(fConfig.DoSaveImages())
{
  // Constructor. Make the GUI.
  int bin2Dx(0), bin2Dy(0);
  fConfig.Get2DnumberBins(bin2Dx, bin2Dy);
  if( bin2Dx > 0 && bin2Dy > 0 ) {
    gEnv->SetValue("Hist.Binning.2D.x", bin2Dx);
    gEnv->SetValue("Hist.Binning.2D.y", bin2Dy);
    if( fVerbosity > 1 ) {
      cout << "Set 2D default bins to x, y: " << bin2Dx << ", " << bin2Dy << endl;
    }
  }

  if( fConfig.DoPrintOnly() ) {
    fPrintOnly = kTRUE;
    PrintPages();
  } else {
    fPrintOnly = kFALSE;
    CreateGUI(gClient->GetRoot(), 1600, 1200);
  }
}

OnlineGUI::OnlineGUI( const OnlineConfig& config )
  : OnlineGUI(OnlineConfig(config)) {}

void OnlineGUI::CreateGUI( const TGWindow* p, UInt_t w, UInt_t h )
{

  // Open the RootFile.  Die if it doesn't exist.
  //  unless we're watching a file.
  fRootFile = new TFile(fConfig.GetRootFile(), "READ");
  if( !fRootFile->IsOpen() ) {
    cout << "ERROR:  rootfile: " << fConfig.GetRootFile()
         << " does not exist"
         << endl;
    if( fConfig.IsMonitor() ) {
      cout << "Will wait... hopefully.." << endl;
    } else {
      gApplication->Terminate();
    }
  } else {
    fFileAlive = kTRUE;
    runNumber = fConfig.GetRunNumber();
    // Open the Root Trees.  Give a warning if it's not there..
    GetFileObjects();
    GetRootTree();
    GetTreeVars();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }

  }
  TString goldenfilename = fConfig.GetGoldenFile();
  if( !goldenfilename.IsNull() ) {
    fGoldenFile = new TFile(goldenfilename, "READ");
    if( !fGoldenFile->IsOpen() ) {
      cout << "ERROR: goldenrootfile: " << goldenfilename
           << " does not exist.  Oh well, no comparison plots."
           << endl;
      doGolden = kFALSE;
      fGoldenFile=NULL;
    } else {
      doGolden = kTRUE;
    }
  } else {
    doGolden=kFALSE;
    fGoldenFile=NULL;
  }

  // Create the main frame
  fMain = new TGMainFrame(p, w, h);
  fMain->Connect("CloseWindow()", "OnlineGUI", this, "MyCloseWindow()");
  ULong_t lightgreen, lightblue, red, mainguicolor;
  gClient->GetColorByName("lightgreen", lightgreen);
  gClient->GetColorByName("lightblue", lightblue);
  gClient->GetColorByName("red", red);

  Bool_t good_color = kFALSE;
  TString usercolor = fConfig.GetGuiColor();
  if( !usercolor.IsNull() ) {
    good_color = gClient->GetColorByName(usercolor, mainguicolor);
  }

  if( !good_color ) {
    if( !usercolor.IsNull() ) {
      cout << "Bad guicolor (" << usercolor << ").. using default." << endl;
    }
    if( fConfig.IsMonitor() ) {
      // Default background color for Online Monitor
      mainguicolor = lightgreen;
    } else {
      // Default background color for Normal Online Display
      mainguicolor = lightblue;
    }
  }

  fMain->SetBackgroundColor(mainguicolor);

  // Top frame, to hold page buttons and canvas
  fTopframe = new TGHorizontalFrame(fMain, w, UInt_t(h * 0.9));
  fTopframe->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fTopframe, new TGLayoutHints(kLHintsExpandX
                                               | kLHintsExpandY, 10, 10, 10, 1));

  // Create a verticle frame widget
  //  This will hold the listbox
  vframe = new TGVerticalFrame(fTopframe, UInt_t(w * 0.3), UInt_t(h * 0.9));
  vframe->SetBackgroundColor(mainguicolor);
  current_page = 0;

  // Create the listbox that'll hold the list of pages
  fPageListBox = new TGListBox(vframe);
  fPageListBox->IntegralHeight(kTRUE);

  TString buff;
  for( UInt_t i = 0; i < fConfig.GetPageCount(); i++ ) {
    buff = fConfig.GetPageTitle(i);
    fPageListBox->AddEntry(buff, i);
  }

  vframe->AddFrame(fPageListBox, new TGLayoutHints(kLHintsExpandX |
                                                   kLHintsCenterY, 5, 5, 3, 4));

  UInt_t maxsize = (fConfig.GetPageCount() + 1 > 30) ? 30 : fConfig.GetPageCount() + 1;
  fPageListBox->Resize(UInt_t(w * 0.15),
                       fPageListBox->GetItemVsize() * (maxsize));

  fPageListBox->Select(0);
  fPageListBox->Connect("Selected(Int_t)", "OnlineGUI", this,
                        "DoListBox(Int_t)");

  // heartbeat below the picture, watchfile only
  if( fConfig.IsMonitor() ) {
    fRootFileLastUpdated = new TGTextButton(vframe, "File updated at: XX:XX:XX");
    // fRootFileLastUpdated->SetWidth(156);
    vframe->AddFrame(fRootFileLastUpdated, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));

    fLastUpdated = new TGTextButton(vframe, "Plots updated at: XX:XX:XX");
    // fLastUpdated->SetWidth(156);
    vframe->AddFrame(fLastUpdated, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));

    fNow = new TGTextButton(vframe, "Current time: XX:XX:XX");
    // fNow->SetWidth(156);
    vframe->AddFrame(fNow, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));
  }

  if( !fConfig.IsMonitor() ) {
    wile =
      new TGPictureButton(vframe, gClient->GetPicture((fConfig.GetGuiDirectory() + "/genius.xpm").c_str()));
    wile->Connect("Pressed()", "OnlineGUI", this, "DoDraw()");
  } else {
    wile =
      new TGPictureButton(vframe, gClient->GetPicture((fConfig.GetGuiDirectory() + "/panguin.xpm").c_str()));
    wile->Connect("Pressed()", "OnlineGUI", this, "DoDrawClear()");
  }
  wile->SetBackgroundColor(mainguicolor);

  vframe->AddFrame(wile, new TGLayoutHints(kLHintsBottom | kLHintsLeft, 5, 10, 4, 2));


  fTopframe->AddFrame(vframe, new TGLayoutHints(kLHintsLeft |
                                                kLHintsCenterY, 2, 2, 2, 2));

  // Create canvas widget
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas", fTopframe, UInt_t(w * 0.7), UInt_t(h * 0.9));
  fEcanvas->SetBackgroundColor(mainguicolor);
  fTopframe->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 1));
  fCanvas = fEcanvas->GetCanvas();

  // Create the bottom frame.  Contains control buttons
  fBottomFrame = new TGHorizontalFrame(fMain, w, UInt_t(h * 0.1));
  fBottomFrame->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fBottomFrame, new TGLayoutHints(kLHintsExpandX, 10, 10, 10, 10));

  // Create a horizontal frame widget with buttons
  hframe = new TGHorizontalFrame(fBottomFrame, 1200, 40);
  hframe->SetBackgroundColor(mainguicolor);
  //fBottomFrame->AddFrame(hframe,new TGLayoutHints(kLHintsExpandX,200,20,2,2));
  fBottomFrame->AddFrame(hframe, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));

  fPrev = new TGTextButton(hframe, "Prev");
  fPrev->SetBackgroundColor(mainguicolor);
  fPrev->Connect("Clicked()", "OnlineGUI", this, "DrawPrev()");
  hframe->AddFrame(fPrev, new TGLayoutHints(kLHintsCenterX, 5, 5, 1, 1));

  fNext = new TGTextButton(hframe, "Next");
  fNext->SetBackgroundColor(mainguicolor);
  fNext->Connect("Clicked()", "OnlineGUI", this, "DrawNext()");
  hframe->AddFrame(fNext, new TGLayoutHints(kLHintsCenterX, 5, 5, 1, 1));

  fExit = new TGTextButton(hframe, "Exit GUI");
  fExit->SetBackgroundColor(red);
  fExit->Connect("Clicked()", "OnlineGUI", this, "CloseGUI()");

  hframe->AddFrame(fExit, new TGLayoutHints(kLHintsCenterX, 5, 5, 1, 1));

  TString Buff;
  if( runNumber == 0 ) {
    Buff = "";
  } else {
    Buff = "Run #";
    Buff += runNumber;
  }
  TGString labelBuff(Buff);

  fRunNumber = new TGLabel(hframe, Buff);
  fRunNumber->SetBackgroundColor(mainguicolor);
  hframe->AddFrame(fRunNumber, new TGLayoutHints(kLHintsCenterX, 5, 5, 1, 1));

  fPrint = new TGTextButton(hframe, "Print To &File");
  fPrint->SetBackgroundColor(mainguicolor);
  fPrint->Connect("Clicked()", "OnlineGUI", this, "PrintToFile()");
  hframe->AddFrame(fPrint, new TGLayoutHints(kLHintsCenterX, 5, 5, 1, 1));


  // Set a name to the main frame
  if( fConfig.IsMonitor() ) {
    fMain->SetWindowName("Parity ANalysis GUI moNitor");
  } else {
    fMain->SetWindowName("Online Analysis GUI");
  }

  // Map all sub windows to main frame
  fMain->MapSubwindows();

  // Initialize the layout algorithm
  fMain->Resize(fMain->GetDefaultSize());

  // Map main frame
  fMain->MapWindow();

  if( fVerbosity >= 1 )
    fMain->Print();

  if( fFileAlive ) DoDraw();

  if( fConfig.IsMonitor() ) {
    timerNow = new TTimer();
    timerNow->Connect(timerNow, "Timeout()", "OnlineGUI", this, "UpdateCurrentTime()");
    timerNow->Start(1000);  // update every second
  }

  if( fConfig.IsMonitor() ) {
    timer = new TTimer();
    if(fFileAlive) {
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"TimerUpdate()");
    } else {
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    }
    timer->Start(UPDATETIME);
  }

}

void OnlineGUI::DoDraw()
{
  // The main Drawing Routine.

  gStyle->SetOptStat(1110);
  //gStyle->SetStatFontSize(0.1);
  if( fConfig.IsLogy(current_page) ) {
    gStyle->SetOptLogy(1);
  } else {
    gStyle->SetOptLogy(0);
  }
  //   gStyle->SetTitleH(0.10);
  //   gStyle->SetTitleW(0.40);
  gStyle->SetTitleH(0.1);
  //  gStyle->SetTitleX(0.55);
  //gStyle->SetTitleW(0.6);
  //gStyle->SetTitleW(0.60);
  //gStyle->SetStatH(0.2);
  gStyle->SetStatW(0.25);
  gStyle->SetStatX(0.9);
  gStyle->SetStatY(0.88);
  //   gStyle->SetLabelSize(0.10,"X");
  //   gStyle->SetLabelSize(0.10,"Y");
  gStyle->SetLabelSize(0.05, "X");
  gStyle->SetLabelSize(0.05, "Y");
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetPadBottomMargin(0.08);
  gStyle->SetPadRightMargin(0.1);
  gStyle->SetPadTopMargin(0.12);
  // gStyle->SetNdivisions(505,"X");
  // gStyle->SetNdivisions(404,"Y");
  // gROOT->ForceStyle();

  TGaxis::SetMaxDigits(3);

  gStyle->SetNdivisions(505, "XYZ");
  gROOT->ForceStyle();

  // Determine the dimensions of the canvas..
  UInt_t draw_count = fConfig.GetDrawCount(current_page);
  if( draw_count >= 8 ) {
    gStyle->SetLabelSize(0.08, "X");
    gStyle->SetLabelSize(0.08, "Y");
  }
  //   Int_t dim = Int_t(round(sqrt(double(draw_count))));
  pair<UInt_t, UInt_t> dim = fConfig.GetPageDim(current_page);

  if( fVerbosity >= 1 )
    cout << "Dimensions: " << dim.first << "X"
         << dim.second << endl;

  // Create a nice clean canvas.
  fCanvas->Clear();
  fCanvas->Divide(dim.first, dim.second);

  //  vector <TString> drawcommand(5);
  map<TString,TString> drawcommand;
  //options are "variable", "cut", "drawopt", "title", "treename", "grid", "nostat"

  // Draw the histograms.
  for( UInt_t i = 0; i < draw_count; i++ ) {
    fConfig.GetDrawCommand(current_page, i, drawcommand);
    fCanvas->cd(i + 1);

    if( drawcommand.find("variable") != drawcommand.end() ) {
      if( drawcommand["variable"] == "macro" ) {
        MacroDraw(drawcommand);
      } else if( drawcommand["variable"] == "loadmacro" ) {
        LoadDraw(drawcommand);
      } else if( drawcommand["variable"] == "loadlib" ) {
        LoadLib(drawcommand);
      } else if( IsHistogram(drawcommand["variable"]) ) {
        HistDraw(drawcommand);
      } else {
        TreeDraw(drawcommand);
      }
    }
  }

  fCanvas->cd();
  fCanvas->Update();

  if( fConfig.IsMonitor() ) {
    char buffer[9]; // HH:MM:SS
    time_t t = time(0);
    TString sLastUpdated("Plots updated at: ");
    strftime(buffer, 9, "%T", localtime(&t));
    sLastUpdated += buffer;
    fLastUpdated->SetText(sLastUpdated);

    struct stat result{};
    stat(fConfig.GetRootFile(), &result);
    time_t tf = result.st_mtime;
    strftime(buffer, 9, "%T", localtime(&tf));

    TString sRootFileLastUpdated("File updated at: ");
    sRootFileLastUpdated += buffer;
    fRootFileLastUpdated->SetText(sRootFileLastUpdated);
    ULong_t backgroundColor = fLastUpdated->GetBackground();
    if( fVerbosity >= 4 )
      cout << "Updating plots (current, file, diff[s]):\t"
           << t << "\t" << tf << "\t" << t - tf << endl;
    if( t - tf > 60 ) {
      ULong_t red;
      gClient->GetColorByName("red", red);
      fRootFileLastUpdated->SetBackgroundColor(red);
    } else {
      fRootFileLastUpdated->SetBackgroundColor(backgroundColor);
    }
  }

  if( !fPrintOnly ) {
    CheckPageButtons();
  }

}

void OnlineGUI::DrawNext()
{
  // Handler for the "Next" button.

  Int_t current_selection = fPageListBox->GetSelected();
  fPageListBox->Select(current_selection + 1);
  current_page = current_selection + 1;

  DoDraw();
}

void OnlineGUI::DrawPrev()
{
  // Handler for the "Prev" button.
  Int_t current_selection = fPageListBox->GetSelected();
  fPageListBox->Select(current_selection - 1);
  current_page = current_selection - 1;

  DoDraw();
}

void OnlineGUI::DoListBox( Int_t id )
{
  // Handle selection in the list box
  current_page = id;
  DoDraw();

}

void OnlineGUI::CheckPageButtons()
{
  // Checks the current page to see if it's the first or last page.
  //  If so... turn off the appropriate button.
  //  If not.. turn on both buttons.

  if( current_page == 0 ) {
    fPrev->SetState(kButtonDisabled);
    if( fConfig.GetPageCount() != 1 )
      fNext->SetState(kButtonUp);
  } else if( current_page == fConfig.GetPageCount() - 1 ) {
    fNext->SetState(kButtonDisabled);
    if( fConfig.GetPageCount() != 1 )
      fPrev->SetState(kButtonUp);
  } else {
    fPrev->SetState(kButtonUp);
    fNext->SetState(kButtonUp);
  }
}

Bool_t OnlineGUI::IsHistogram(TString objectname)
{
  // Utility to determine if the objectname provided is a histogram

  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(objectname)) {
      if(fVerbosity>=2)
	cout << fileObjects[i].first << "      "
	     << fileObjects[i].second << endl;

      if(fileObjects[i].second.Contains("TH"))
	return kTRUE;
    }
  }
  return kFALSE;
}

void OnlineGUI::GetFileObjects()
{
  // Utility to find all of the objects within a File (TTree, TH1F, etc).
  //  The pair stored in the vector is <ObjName, ObjType>
  //  If there's no good keys.. do nothing.
  if( fVerbosity >= 1 )
    cout << "Keys = " << fRootFile->ReadKeys() << endl;

  if( fRootFile->ReadKeys() == 0 ) {
    fUpdate = kFALSE;
    //     delete fRootFile;
    //     fRootFile = 0;
    //     CheckRootFile();
    return;
  }
  fileObjects.clear();
  TIter next(fRootFile->GetListOfKeys());
  TKey *key = new TKey();

  // Do the search
  while( (key = (TKey*) next()) ) {
    if( fVerbosity >= 1 )
      cout << "Key = " << key << endl;

    TString objname = key->GetName();
    TString objtype = key->GetClassName();

    if( fVerbosity >= 1 )
      cout << objname << " " << objtype << endl;

    fileObjects.push_back(make_pair(objname,objtype));
  }
  fUpdate = kTRUE;
  delete key;
}

void OnlineGUI::GetTreeVars()
{
  // Utility to find all of the variables (leaf's/branches) within a
  // Specified TTree and put them within the treeVars vector.
  treeVars.clear();
  TObjArray* branchList;
  vector<TString> currentTree;

  for(UInt_t i=0; i<fRootTree.size(); i++) {
    currentTree.clear();
    branchList = fRootTree[i]->GetListOfBranches();
    TIter next(branchList);
    TBranch* brc;

    while((brc=(TBranch*)next())!=0) {
      TString found = brc->GetName();
      // Not sure if the line below is so smart...
      currentTree.push_back(found);
    }
    treeVars.push_back(currentTree);
  }

  if( fVerbosity >= 5 ) {
    for( UInt_t iTree = 0; iTree < treeVars.size(); iTree++ ) {
      cout << "In Tree " << iTree << ": " << endl;
      for(UInt_t i=0; i<treeVars[iTree].size(); i++) {
	cout << treeVars[iTree][i] << endl;
      }
    }
  }
}


void OnlineGUI::GetRootTree()
{
  // Utility to search a ROOT File for ROOT Trees
  // Fills the fRootTree vector
  fRootTree.clear();

  list <TString> found;
  for(UInt_t i=0; i<fileObjects.size(); i++) {

    if(fVerbosity>=2)
      cout << "Object = " << fileObjects[i].second <<
	"     Name = " << fileObjects[i].first << endl;

    if(fileObjects[i].second.Contains("TTree"))
      found.push_back(fileObjects[i].first);
  }

  // Remove duplicates, then insert into fRootTree
  found.unique();
  UInt_t nTrees = found.size();

  for( UInt_t i = 0; i < nTrees; i++ ) {
    fRootTree.push_back((TTree*) fRootFile->Get(found.front()));
    found.pop_front();
  }
  // Initialize the fTreeEntries vector
  fTreeEntries.clear();
  for( UInt_t i = 0; i < fRootTree.size(); i++ ) {
    fTreeEntries.push_back(0);
  }
}

UInt_t OnlineGUI::GetTreeIndex( TString var )
{
  // Utility to find out which Tree (in fRootTree) has the specified
  // variable "var".  If the variable is a collection of Tree
  // variables (e.g. bcm1:lumi1), will only check the first
  // (e.g. bcm1).
  // Returns the correct index.  if not found returns an index 1
  // larger than fRootTree.size()

  //  This is for 2d draws... look for the first only
  string svar{var.Data()};
  auto pos = svar.find_first_of(":-/*+([");
  if( pos != string::npos )
    svar.erase(pos);

  if( fVerbosity >= 3 )
    cout << __PRETTY_FUNCTION__ << "\t" << __LINE__ << endl
         << "\t looking for variable: " << svar << endl;

  for( UInt_t iTree = 0; iTree < treeVars.size(); iTree++ ) {
    for( UInt_t ivar = 0; ivar < treeVars[iTree].size(); ivar++ ) {
      if( fVerbosity >= 4 )
        cout << "Checking tree " << iTree << " name:" << fRootTree[iTree]->GetName()
             << " \t var " << ivar << " >> " << treeVars[iTree][ivar] << endl;
      if( svar == treeVars[iTree][ivar] ) return iTree;
    }
  }

  return fRootTree.size() + 1;
}

UInt_t OnlineGUI::GetTreeIndexFromName(TString name) {
  // Called by TreeDraw().  Tries to find the Tree index provided the
  //  name.  If it doesn't match up, return a number that's one larger
  //  than the number of found trees.
  for( UInt_t iTree = 0; iTree < fRootTree.size(); iTree++ ) {
    TString treename = fRootTree[iTree]->GetName();
    if( name == treename ) {
      return iTree;
    }
  }

  return fRootTree.size() + 1;
}

void OnlineGUI::MacroDraw(std::map<TString,TString> &command) {
  // Called by DoDraw(), this will make a call to the defined macro, and
  //  plot it in it's own pad.  One plot per macro, please.

  if(command.find("macro") == command.end() ){
    cout << "macro command doesn't contain a macro to execute" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gROOT->Macro(command["macro"]);


}

void OnlineGUI::LoadDraw(std::map<TString,TString> &command) {
  // Called by DoDraw(), this will load a shared object library
  // and then make a call to the defined macro, and
  // plot it in it's own pad.  One plot per macro, please.

  //  TString slib("library");
  //TString smacro("macro");

  if(command.find("library") == command.end() ||
     command.find("macro") == command.end() ) {
    cout << "load command is missing either a shared library or macro command or both" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gSystem->Load(command["library"]);
  gROOT->Macro(command["macro"]);


}

void OnlineGUI::LoadLib(std::map<TString,TString> &command) {
  // Called by DoDraw(), this will load a shared object library

  if(command.find("library") == command.end() ) {
    cout << "load command doesn't contain a shared object library path" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gSystem->Load(command["library"]);


}

void OnlineGUI::DoDrawClear()
{
  // Utility to grab the number of entries in each tree.  This info is
  // then used, if watching a file, to "clear" the TreeDraw
  // histograms, and begin looking at new data.
  for( UInt_t i = 0; i < fTreeEntries.size(); i++ ) {
    fTreeEntries[i] = (Int_t) fRootTree[i]->GetEntries();
  }
}

void OnlineGUI::TimerUpdate()
{
  // Called periodically by the timer, if "watchfile" is indicated
  // in the config.  Reloads the ROOT file, and updates the current page.
  if( fVerbosity >= 1 )
    cout << __PRETTY_FUNCTION__ << "\t" << __LINE__ << endl;

#ifdef OLDTIMERUPDATE
  if( fVerbosity >= 2 )
    cout << "\t rtFile: " << fRootFile << "\t" << fConfig.GetRootFile() << endl;
  if( fRootFile ) {
    fRootFile->Close();
    fRootFile->Delete();
    delete fRootFile;
    fRootFile=0;
  }
  fRootFile = new TFile(fConfig.GetRootFile(), "READ");
  if( fRootFile->IsZombie() ) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return;
  }

  // Update the runnumber
  runNumber = fConfig.GetRunNumber();
  if( runNumber != 0 ) {
    TString rnBuff = "Run #";
    rnBuff += runNumber;
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }

  // Open the Root Trees.  Give a warning if it's not there..
  GetFileObjects();
  if (fUpdate) { // Only do this stuff if their are valid keys
    GetRootTree();
    GetTreeVars();
    for( UInt_t i = 0; i < fRootTree.size(); i++ ) {
      if( !fRootTree[i] ) {
        fRootTree.erase(fRootTree.begin() + i);
      }
    }
    DoDraw();
  }
  timer->Reset();

#else

  if(fRootFile->IsZombie() || (fRootFile->GetSize() == -1)
     || (fRootFile->ReadKeys()==0)) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return;
  }
  for(UInt_t i=0; i<fRootTree.size(); i++) {
    fRootTree[i]->Refresh();
  }
  DoDraw();
  timer->Reset();

#endif

}

void OnlineGUI::UpdateCurrentTime()
{
  char buffer[9];
  time_t t = time(0);
  strftime(buffer, 9, "%T", localtime(&t));
  TString sNow("Current time: ");
  sNow += buffer;
  fNow->SetText(sNow);
  timerNow->Reset();
}

void OnlineGUI::BadDraw(TString errMessage) {
  // Routine to display (in Pad) why a particular draw method has
  // failed.
  auto* pt = new TPaveText(0.1, 0.1, 0.9, 0.9, "brNDC");
  pt->SetBorderSize(3);
  pt->SetFillColor(10);
  pt->SetTextAlign(22);
  pt->SetTextFont(72);
  pt->SetTextColor(2);
  pt->AddText(errMessage.Data());
  pt->Draw();
  //   cout << errMessage << endl;
}


void OnlineGUI::CheckRootFile()
{
  // Check the path to the rootfile (should follow symbolic links)
  // ... If found:
  //   Reopen new root file,
  //   Reconnect the timer to TimerUpdate()

  if( gSystem->AccessPathName(fConfig.GetRootFile()) == 0 ) {
    cout << "Found the new run" << endl;
#ifndef OLDTIMERUPDATE
    if(OpenRootFile()==0) {
#endif
      timer->Reset();
      timer->Disconnect();
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"TimerUpdate()");
#ifndef OLDTIMERUPDATE
    }
#endif
  } else {
    TString rnBuff = "Waiting for run";
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }
}

Int_t OnlineGUI::OpenRootFile()
{


  fRootFile = new TFile(fConfig.GetRootFile(), "READ");
  if( fRootFile->IsZombie() || (fRootFile->GetSize() == -1)
      || (fRootFile->ReadKeys() == 0) ) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return -1;
  }

  // Update the runnumber
  runNumber = fConfig.GetRunNumber();
  if( runNumber != 0 ) {
    TString rnBuff = "Run #";
    rnBuff += runNumber;
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }

  // Open the Root Trees.  Give a warning if it's not there..
  GetFileObjects();
  if (fUpdate) { // Only do this stuff if their are valid keys
    GetRootTree();
    GetTreeVars();
    for( UInt_t i = 0; i < fRootTree.size(); i++ ) {
      if( !fRootTree[i] ) {
        fRootTree.erase(fRootTree.begin() + i);
      }
    }
    DoDraw();
  } else {
    return -1;
  }
  return 0;

}
void OnlineGUI::SaveImage(TObject* o,std::map<TString,TString> &command)
{
  if(this->fSaveImages)
      {
        cout<<"saving image "<< command["variable"] <<endl;
        TCanvas *c = new TCanvas("c","c",gPad->GetWw(),gPad->GetWh());
        o->Draw(command["drawopt"]);
        c->Print("hydra_"+command["variable"]+".png");
        delete c;
      }
}
void OnlineGUI::HistDraw(std::map<TString,TString> &command) {
  // Called by DoDraw(), this will plot a histogram.

  Bool_t showGolden = kFALSE;
  if( doGolden ) showGolden = kTRUE;

  if( command.find("noshowgolden") != command.end() ) {
    showGolden = kFALSE;
  }
  // cout<<"showGolden= "<<showGolden<<endl;

  TString drawopt = "";
  if( command.find("drawopt") != command.end() ){
    drawopt = command["drawopt"];
  }

  TString newtitle = "";
  if( command.find("title") != command.end() ){
    newtitle = command["title"];
  }

  if( command.find("logx") != command.end() ) {
    gPad->SetLogx();
  }

  if( command.find("logy") != command.end() ) {
    gPad->SetLogy();
  }

  if( command.find("logz") != command.end() ) {
    gPad->SetLogz();
  }

  bool showstat = true;
  if( command.find("nostat") != command.end() ) {
    showstat = false;
  }

  // Determine dimensionality of histogram
  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(command["variable"])) {
      if(fileObjects[i].second.Contains("TH1")) {
	if(showGolden) fRootFile->cd();
	mytemp1d = (TH1D*)gDirectory->Get(command["variable"]);
	if(mytemp1d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else {
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp1d_golden = (TH1D*)gDirectory->Get(command["variable"]);
	    mytemp1d_golden->SetLineColor(30);
	    mytemp1d_golden->SetFillColor(30);
	    Int_t fillstyle=3027;
	    if(fPrintOnly) fillstyle=3010;
	    mytemp1d_golden->SetFillStyle(fillstyle);
	    mytemp1d_golden->SetStats(false);
	    if( newtitle != "" ) mytemp1d_golden->SetTitle(newtitle);
	    mytemp1d_golden->Draw();
	    cout<<"one golden histo drawn"<<endl;
	    mytemp1d->SetStats(showstat);
	    mytemp1d->Draw("sames"+drawopt);
	  } else {
	    mytemp1d->SetStats(showstat);
	    if( newtitle != "" ) mytemp1d->SetTitle(newtitle);
	    mytemp1d->Draw(drawopt);

      SaveImage(mytemp1d, command);
	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH2")) {
	if(showGolden) fRootFile->cd();
	mytemp2d = (TH2D*)gDirectory->Get(command["variable"]);
	if(mytemp2d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else {
	  // These are commented out for some reason (specific to DVCS?)
	  // 	  if(showGolden) {
	  // 	    fGoldenFile->cd();
	  // 	    mytemp2d_golden = (TH2D*)gDirectory->Get(command["variable"]);
	  // 	    mytemp2d_golden->SetMarkerColor(2);
	  // 	    mytemp2d_golden->Draw();
	  //mytemp2d->Draw("sames");
	  // 	  } else { because it usually doesn't make sense to superimpose two 2d histos together:

	  if( drawopt.Contains("colz") ){
	    gPad->SetRightMargin(0.15);
	  }

	  if( newtitle != "" ) mytemp2d->SetTitle(newtitle);
	  mytemp2d->SetStats(showstat);
	  mytemp2d->Draw(drawopt);
    SaveImage(mytemp2d, command);
	  // 	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH3")) {
	if(showGolden) fRootFile->cd();
	mytemp3d = (TH3D*)gDirectory->Get(command["variable"]);
	if(mytemp3d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else {
	  mytemp3d->Draw();
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp3d_golden = (TH3D*)gDirectory->Get(command["variable"]);
	    mytemp3d_golden->SetMarkerColor(2);
	    mytemp3d_golden->Draw();
	    mytemp3d->Draw("sames"+drawopt);
	  } else {
	    mytemp3d->Draw(drawopt);
	  }

    SaveImage(mytemp3d, command);
	}
	break;
      }
    }
  }

}

void OnlineGUI::TreeDraw(map<TString,TString> &command) {
  // Called by DoDraw(), this will plot a Tree Variable

  TString var = command["variable"];

  //  Check to see if we're projecting to a specific histogram
  TString histoname = command["variable"](TRegexp(">>.+(?"));
  if (histoname.Length()>0){
    histoname.Remove(0,2);
    Int_t bracketindex = histoname.First("(");
    if (bracketindex>0) histoname.Remove(bracketindex);
    if(fVerbosity>=3)
      std::cout << histoname << " "<< command["variable"](TRegexp(">>.+(?")) <<std::endl;
  } else {
    histoname = "htemp";
  }

  // Combine the cuts (definecuts and specific cuts)
  TCut cut = "";
  TString tempCut;
  if(command.size()>1) {
    tempCut = command["cut"];
    vector <TString> cutIdents = fConfig.GetCutIdent();
    for(UInt_t i=0; i<cutIdents.size(); i++) {
      if(tempCut.Contains(cutIdents[i])) {
	TString cut_found = (TString)fConfig.GetDefinedCut(cutIdents[i]);
	tempCut.ReplaceAll(cutIdents[i],cut_found);
      }
    }
    cut = (TCut) tempCut;
  }

  // Determine which Tree the variable comes from, then draw it.
  UInt_t iTree;
  if(command["tree"].IsNull()) {
    iTree = GetTreeIndex(var);
    if( fVerbosity >= 2 )
      cout << "got index from variable " << iTree << endl;
  } else {
    iTree = GetTreeIndexFromName(command["tree"]);
    if(fVerbosity>=2)
      cout<<"got index from command "<<iTree<<endl;
  }
  TString drawopt = command["drawopt"];

  std::cout << "drawopt = " << drawopt << std::endl;

  if( drawopt.Contains("colz") ) gPad->SetRightMargin(0.15);

  if(fVerbosity>=3)
    cout<<"\tDraw option:"<<drawopt<<" and histo name "<<histoname<<endl;
  Int_t errcode=0;
  if (iTree <= fRootTree.size() ) {
    if(fVerbosity>=1){
      cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
      cout<<command["variable"]<<"\t"<<command["cut"]<<"\t"<<command["drawopt"]<<"\t"<<command[3]
	  <<"\t"<<command["tree"]<<endl;
      if(fVerbosity>=2)
	cout<<"\tProcessing from tree: "<<iTree<<"\t"<<fRootTree[iTree]->GetTitle()<<"\t"
	    <<fRootTree[iTree]->GetName()<<endl;
    }
    errcode = fRootTree[iTree]->Draw(var,cut,drawopt);
    if (command["grid"].EqualTo("grid")){
      gPad->SetGrid();
    }

    TObject *hobj = (TObject*)gROOT->FindObject(histoname);
    if(fVerbosity>=3)
      cout<<"Finished drawing with error code "<<errcode<<endl;

    if(errcode==-1) {
      BadDraw(var+" not found");
    } else if (errcode!=0) {
      if(!command[3].IsNull()) {
        //  Generate a "unique" histogram name based on the MD5 of the drawn variable, cut, drawopt,
        //  and plot title.
        //  Makes it less likely to cause a name collision if two plot titles are the same.
        //  If you draw the exact same plot twice, the histograms will have the same name, but
        //  since they are exactly the same, you likely won't notice (or it will complain at you).
        TString tmpstring(var);
        tmpstring += cut.GetTitle();
        tmpstring += drawopt;
        tmpstring += command["title"];
        TString myMD5 = tmpstring.MD5();
	TH1* thathist = (TH1*)hobj;
	thathist->SetNameTitle(myMD5,command["title"]);
  SaveImage(thathist, command);
      }
    } else {
      BadDraw("Empty Histogram");
    }
  } else {
    BadDraw(var + " not found");
    if( fConfig.IsMonitor() ) {
      // Maybe we missed it... look again.  I don't like the code
      // below... maybe I can come up with something better
      GetFileObjects();
      GetRootTree();
      GetTreeVars();
    }
  }
}

void OnlineGUI::PrintToFile()
{
  // Routine to print the current page to a File.
  //  A file dialog pops up to request the file name.
  fCanvas = fEcanvas->GetCanvas();
  gStyle->SetPaperSize(20, 24);
  static TString dir("printouts");
  TGFileInfo fi;
  const char* myfiletypes[] =
    {"All files", "*",
     "Portable Document Format", "*.pdf",
     "PostScript files", "*.ps",
     "Encapsulated PostScript files", "*.eps",
     "GIF files", "*.gif",
     "JPG files", "*.jpg",
     nullptr, nullptr};
  fi.fFileTypes = myfiletypes;
  fi.fIniDir    = StrDup(dir.Data());

  new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
  if( fi.fFilename ) fCanvas->Print(fi.fFilename);
}

void OnlineGUI::PrintPages()
{
  // Routine to go through each defined page, and print the output to
  // a postscript file. (good for making sample histograms).

  // Open the RootFile
  //  unless we're watching a file.
  fRootFile = new TFile(fConfig.GetRootFile(), "READ");
  if( !fRootFile->IsOpen() ) {
    cout << "ERROR:  rootfile: " << fConfig.GetRootFile()
         << " does not exist"
         << endl;
    gApplication->Terminate();
  } else {
    fFileAlive = kTRUE;
    GetFileObjects();
    GetRootTree();
    GetTreeVars();
    for( UInt_t i = 0; i < fRootTree.size(); i++ ) {
      if( !fRootTree[i] ) {
        fRootTree.erase(fRootTree.begin() + i);
      }
    }
  }
  TString goldenfilename = fConfig.GetGoldenFile();
  if( !goldenfilename.IsNull() ) {
    fGoldenFile = new TFile(goldenfilename, "READ");
    if( !fGoldenFile->IsOpen() ) {
      cout << "ERROR: goldenrootfile: " << goldenfilename
           << " does not exist.  Oh well, no comparison plots."
           << endl;
      doGolden = kFALSE;
      fGoldenFile = nullptr;
    } else {
      doGolden = kTRUE;
    }
  } else {
    doGolden = kFALSE;
    fGoldenFile = nullptr;
  }

  fCanvas = new TCanvas("fCanvas", "trythis", 1000, 800);
  auto* lt = new TLatex();

  TString plotsdir = fConfig.GetPlotsDir();
  if( plotsdir.IsNull() ) plotsdir = ".";

  Bool_t pagePrint = kFALSE;
  TString printFormat = fConfig.GetPlotFormat();
  cout << "Plot Format = " << printFormat << endl;
  if( printFormat.IsNull() ) printFormat = "pdf";
  if( printFormat != "pdf" ) pagePrint = kTRUE;

  TString filename = "summaryPlots";
  runNumber = fConfig.GetRunNumber();
  if( runNumber != 0 ) {
    filename += "_";
    filename += runNumber;
  } else {
    printf(" Warning for pretty plots: runNumber = %i\n", runNumber);
  }

  filename.Prepend(plotsdir + "/");
  if( pagePrint )
    filename += "_pageXXXX";
  TString fConfName = fConfig.GetConfFileName();
  TString fCfgNm = fConfName(fConfName.Last('/') + 1, fConfName.Length());
  filename += "_" + fCfgNm(0, fCfgNm.Last('.'));
  filename += "." + printFormat;

  TString pagehead = "Summary Plots";
  if( runNumber != 0 ) {
    pagehead += "(Run #";
    pagehead += runNumber;
    pagehead += ")";
  }
  //  pagehead += ": ";

  gStyle->SetPalette(1);
  //gStyle->SetTitleX(0.15);
  //gStyle->SetTitleY(0.9);
  gStyle->SetPadBorderMode(0);
  //gStyle->SetHistLineColor(1);
  gStyle->SetHistFillStyle(0);
  if( !pagePrint ) fCanvas->Print(filename + "[");
  TString origFilename = filename;
  for( UInt_t i = 0; i < fConfig.GetPageCount(); i++ ) {
    current_page = i;
    DoDraw();
    TString pagename = pagehead;
    pagename += " ";
    pagename += i;
    pagename += ": ";
    pagename += fConfig.GetPageTitle(current_page);
    lt->SetTextSize(0.025);
    lt->DrawLatex(0.05, 0.98, pagename);
    if( pagePrint ) {
      filename = origFilename;
      filename.ReplaceAll("XXXX", Form("%02d", current_page));
      cout << "Printing page " << current_page
           << " to file = " << filename << endl;
    }
    fCanvas->Print(filename);
  }
  if( !pagePrint ) fCanvas->Print(filename + "]");

  gApplication->Terminate();
}

void OnlineGUI::MyCloseWindow()
{
  fMain->SendCloseMessage();
  cout << "OnlineGUI Closed." << endl;
  if( timer ) {
    timer->Stop();
    delete timer;
  }
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  delete fPageListBox;
  delete hframe;
  delete fEcanvas;
  delete fBottomFrame;
  delete vframe;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;

  gApplication->Terminate();
}

void OnlineGUI::CloseGUI()
{
  // Routine to take care of the Exit GUI button
  fMain->SendCloseMessage();
}

OnlineGUI::~OnlineGUI()
{
  //  fMain->SendCloseMessage();
  if( timer ) {
    timer->Stop();
    delete timer;
  }
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  delete fPageListBox;
  delete hframe;
  delete fEcanvas;
  delete vframe;
  delete fBottomFrame;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;
}
