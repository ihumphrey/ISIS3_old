#if !defined(LoadCSV_h)
#define LoadCSV_h
/**                                                                       
 * @file                                                                  
 * $Revision 
 * $Date$
 * $Id$
 * 
 *   Unless noted otherwise, the portions of Isis written by the USGS are 
 *   public domain. See individual third-party library and package descriptions 
 *   for intellectual property information, user agreements, and related  
 *   information.                                                         
 *                                                                        
 *   Although Isis has been used by the USGS, no warranty, expressed or   
 *   implied, is made by the USGS as to the accuracy and functioning of such 
 *   software and related material nor shall the fact of distribution     
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.                                        
 *                                                                        
 *   For additional information, launch                                   
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html                
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.                                    
 */                                                                       
#include <cmath>
#include <string>
#include <vector>

#include "iString.h"
#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalConf.h"
#include "CSVReader.h"

namespace Isis {

  class iException;

  /**
   * @brief Provides generalized access to HiRISE calibration CSV files
   * 
   * This class will load a CSV file and extract rows and/or columns based upon 
   * a HiRISE calibration profile properly configured to define the format of 
   * the CSV file. 
   *  
   * It will utilize the HiCalConf object to extract keywords/parameters from a 
   * base name of a keyword set.  For eaxample, if the a profile contains a CSV 
   * file profile with a keyword called "AMatrix" that specifies the pattern 
   * used to determine the appropriate file, then additional keywords can be 
   * specfied that information about the format of the CSV file.  Other keywords 
   * are:  AMatrixColumnHeader, AMatrixRowHeader, AMatrixColumnName, 
   * AMatrixRowName, AMatrixColumnIndex and AMatrixRowIndex. 
   *  
   * Note that all HiRISE CSV files must conform to this format.  All blank 
   * lines of lines that start with a '#' (comment) are ignored when the CSV 
   * files is read in. 
   *  
   * Note this object is reentrant.  You can load successive CSV files one after
   * the other using the same object. 
   *  
   * @ingroup Utility
   * 
   * @author 2010-04-06 Kris Becker
   */
  class LoadCSV {

    public: 
      //  Constructors and Destructor
      LoadCSV();
      LoadCSV(const std::string &base, const HiCalConf &conf,
              const DbProfile &profile);

      /** Destructor */
      virtual ~LoadCSV() { }

      void load(const std::string &base, const HiCalConf &conf,
               const DbProfile &profile) throw (iException &);

      std::string filename() const;
      int size() const;

      bool validateSize(const int &expected,
                        const bool &throw_on_error = false) 
                        const throw (iException &);

      HiVector getVector() const;
      HiMatrix getMatrix() const;

      void History(HiHistory &history) const;

    private:
      std::string              _base;
      DbProfile                _csvSpecs;
      HiMatrix                 _data;
      std::vector<std::string> _history;

      void init(const std::string &base,  const HiCalConf &conf,
                const DbProfile &profile);
      void addHistory(const std::string &element, const std::string &desc);
      void getKeyList(const std::string &base, std::vector<std::string> &keys) 
                      const;
      DbProfile ResolveKeys(const std::string &base, const HiCalConf &conf,
                            const DbProfile &prof) const;
      std::string ParsedKey(const std::string &key, const HiCalConf &conf,
                            const DbProfile &prof) const;
      std::string makeKey(const std::string &ksuffix = "") const;
      std::string getValue(const std::string &ksuffix = "") const;
      HiMatrix extract (const CSVReader &csv);
      int getAxisIndex(const std::string &name,
                       const CSVReader::CSVAxis &header) const;

  };

}     // namespace Isis
#endif

