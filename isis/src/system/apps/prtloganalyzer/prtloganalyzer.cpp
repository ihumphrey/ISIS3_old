// $Id$
#include "Isis.h"
#include "FileList.h"
#include "Filename.h"
#include "Pvl.h"
#include "Process.h"
#include "ProgramAnalyzer.h"
#include "iTime.h"
#include "iException.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  Process p;
  ProgramAnalyzer analyzer;

  // Get the list of names of input CCD cubes to stitch together
  UserInterface &ui = Application::GetUserInterface();

  //  Add program exclusions
  if (ui.WasEntered("EXCLUDE")) analyzer.setExclude(ui.GetString("EXCLUDE"));
  if (ui.WasEntered("EXCLUDEFROM") ) {
    FileList elist(ui.GetFilename("EXCLUDEFROM"));
    for (unsigned int i = 0 ; i < elist.size() ; i++ ) {
      analyzer.exclude(elist[i]);
    }
  }

  // Add program inclusions
  if (ui.WasEntered("INCLUDE")) analyzer.setInclude(ui.GetString("INCLUDE"));
  if (ui.WasEntered("INCLUDEFROM") ) {
    FileList ilist(ui.GetFilename("INCLUDEFROM"));
    for (unsigned int i = 0 ; i < ilist.size() ; i++ ) {
      analyzer.include(ilist[i]);
    }
  }

  // Add the file
  analyzer.add(ui.GetFilename("FROM"));

  //  Log results
  PvlGroup logger = analyzer.review();
  Application::GuiLog(logger);
  Application::Log(logger);

  logger = analyzer.cumulative();
  Application::GuiLog(logger);
  Application::Log(logger);


  // Write the output file if requested for individual unique program summaries
  if(ui.WasEntered("SUMMARY")) {
    Pvl temp;
    temp.AddGroup(analyzer.review());
    temp.AddGroup(analyzer.cumulative());
    for (int i = 0 ; i < analyzer.Programs() ; i++) {
      temp.AddGroup(analyzer.summarize(i));
    }
    temp.Write(ui.GetFilename("SUMMARY"));
  }

  // Write the output file if requested of CSV formatted data
  if(ui.WasEntered("LOG")) {
   // Set up for opening
    Filename temp(ui.GetFilename("LOG"));
    string file = temp.Expanded();
    ofstream ostm;

    // Open the file
    ostm.open(file.c_str(), std::ios::out);
    if(!ostm) {
      string message = "Cannot open/create output file " + file;
      throw iException::Message(iException::Io, message, _FILEINFO_);
    }

    analyzer.header(ostm);
    analyzer.listify(ostm);
    ostm.close();
  }

  p.EndProcess();
}
