// ***************************************************************************
// *                                                                         *
// *          Aflow COREY OSES - Johns Hopkins University 2024-2026          *
// *            Aflow YUXIANG LIU - Johns Hopkins University 2024            *
// *          Aflow TIANHAO LI - Johns Hopkins University 2025-2026          *
// *                                                                         *
// ***************************************************************************
//aflow_pocc_sampling.cpp
//Reading Pocc sampling rate from --pocc_sample_rate and --pocc_sample_number
//Two random seed sampling functions are provided for configuration selections.
//First random seed function is for unique configuration calculations.
//Second random seed function is for DFT calculations.
//2024- Yuxiang Liu
//2025- Tianhao Li

#ifndef _AFLOW_POCC_SAMPLING_CPP_
#define _AFLOW_POCC_SAMPLING_CPP_

#include "aflow.h"
#include "aflow_pocc.h"
#include <unordered_set>  // std::unordered_set //CO20260930
#include <random>  // std::mt19937_64 //CO20261006
#include <set>  // std::set //CO20261006

#define _DEBUG_POCC_SAMPLE_ false 

namespace pocc {

    double POccCalculator::setPOccSampleRate(const string& pocc_sample_rate_string, int sample_round){//give the warning message if --pocc_sample_rate format is wrong
        double pocc_sample_rate=0.0;
        stringstream message;
        //if(pocc_sample_rate_string.empty()){
        //   message << "No POCC random seed sampling rate is found" << endl;
        //   throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
        //}   
        if(aurostd::substring2bool(pocc_sample_rate_string,",")){
           message << "Cannot handle more than one pocc_sample_rate specification";
           throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
        }   
 
        vector<string> tokens;
        aurostd::string2tokens(pocc_sample_rate_string,tokens,":");
        if(tokens.size()<2){
           message << "No second random seed sampling rate is found" << endl;
           throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
        }   
        else{
           if(!aurostd::isfloat(tokens[sample_round])){
             message << "Input is not a float [input=" << tokens[sample_round] << "]";
             throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
           }   
           pocc_sample_rate=aurostd::string2utype<double>(tokens[sample_round]);
           if(aurostd::isequal(pocc_sample_rate,0.0,_AFLOW_POCC_ZERO_TOL_)){
             message << "Sample rate is too small (0) [input=" << tokens[sample_round] << "]";
             throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
           }   
           if(std::signbit(pocc_sample_rate)){
              message << "Sample rate cannot be negative [input=" << tokens[sample_round] << "]";
              throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
           }   
        }  
        return pocc_sample_rate;
    }

    vector<unsigned long long int> POccCalculator::FirstRandomSampling(unsigned long long int hnf_count, unsigned long long int types_config_permutations_count){
         size_t sample_config_count=0;
         bool LDEBUG = (FALSE || _DEBUG_POCC_SAMPLE_ || XHOST.DEBUG);
         stringstream message;
         unsigned long long int sample_number=0;
         string sample_number_string="";
         bool XHOST_POCC_SAMPLE_NUMBER=usePOccSampleNumber(sample_number_string, sample_number);  //TL20260204
         string sample_rate_string="";  //TL20260205
         bool XHOST_POCC_SAMPLE_RATE=usePOccSampleRate(sample_rate_string);  //TL20260205
         string sample_hash_string="";  //TL20260303
         unsigned long long int sample_hash_hnf=AUROSTD_MAX_ULLINT;
         unsigned long long int sample_hash_site_config=AUROSTD_MAX_ULLINT;
         bool XHOST_POCC_SAMPLE_HASH=usePOccSampleHash(sample_hash_string,sample_hash_hnf,sample_hash_site_config);  //TL20260303
         if(LDEBUG){
             cerr << __AFLOW_FUNC__ << " This is a test message" << endl;
             cerr << __AFLOW_FUNC__ << " This is first random seed sampling for unique configuration calculations" << endl;
             cerr << __AFLOW_FUNC__ << " XHOST_POCC_SAMPLE_NUMBER = " << XHOST_POCC_SAMPLE_NUMBER << endl;
             cerr << __AFLOW_FUNC__ << " XHOST_POCC_SAMPLE_RATE = " << XHOST_POCC_SAMPLE_RATE << endl;
             cerr << __AFLOW_FUNC__ << " XHOST_POCC_SAMPLE_HASH = " << XHOST_POCC_SAMPLE_HASH << endl;
             cerr << __AFLOW_FUNC__ << " POCC_SAMPLE_NUMBER_STRING = " << sample_number_string << endl;
             cerr << __AFLOW_FUNC__ << " POCC_SAMPLE_RATE_STRING = " << sample_rate_string << endl;  //TL20260205
             cerr << __AFLOW_FUNC__ << " POCC_SAMPLE_HASH_STRING = " << sample_hash_string << endl;  //TL20260303
         }
         int sample_seed = DEFAULT_POCC_SAMPLE_SEED;
         srand(sample_seed); //initial random seed sampling
         if(XHOST_POCC_SAMPLE_RATE){
             double sample_rate = setPOccSampleRate(sample_rate_string,0);  //TL20260205 use sample_rate_string from helper
             sample_config_count = static_cast<size_t>(types_config_permutations_count * sample_rate);
         } // calculate total amount of selected decoration permutation configurations for each derivative supperlattice
         if(XHOST_POCC_SAMPLE_NUMBER){
             sample_config_count = static_cast<size_t>(sample_number);
         } // calculate total amount of selected decoration permutation configurations for each derivative supperlattice
         if(sample_config_count > static_cast<size_t>(types_config_permutations_count)){
             sample_config_count = static_cast<size_t>(types_config_permutations_count);
         }
         if(LDEBUG){cerr << __AFLOW_FUNC__ << " aflow_pocc_random_seed: " << "sample_config_count" << sample_config_count << endl;}
         std::unordered_set<unsigned long long int> selected_site_config_indices_set; // unordered_set could auotmatically select unique index avoiding repetion and is cheaper than find
         if(XHOST_POCC_SAMPLE_HASH){
             if(sample_hash_hnf>=hnf_count){
                message << "POCC_SAMPLE_HASH H index out of range for this run [POCC_SAMPLE_HASH=" << sample_hash_string << ", HNF_COUNT=" << hnf_count << "]";
                throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
             }
             if(sample_hash_site_config>=types_config_permutations_count){
                message << "POCC_SAMPLE_HASH C index out of sampled search range [POCC_SAMPLE_HASH=" << sample_hash_string << ", TYPES_CONFIG_COUNT=" << types_config_permutations_count << "]";
                throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
             }
             if(sample_config_count==0){
                sample_config_count=1;
             }
             selected_site_config_indices_set.insert(sample_hash_site_config);
         }
         unsigned long long int index; //TL20250612 Declare outside loop to avoid repeated memory allocation
         while(selected_site_config_indices_set.size() < sample_config_count) {
             index = rand() % types_config_permutations_count; //TL20250612 select permuation decoration configuration index
             selected_site_config_indices_set.insert(index);
         }
         std::vector<unsigned long long int> selected_site_config_indices(selected_site_config_indices_set.begin(), selected_site_config_indices_set.end());
         sort(selected_site_config_indices.begin(), selected_site_config_indices.end());//set selected_indices to save sorted indices
         if(LDEBUG)
         {
            cerr << __AFLOW_FUNC__ << "selected_site_config_indices.size: " << selected_site_config_indices.size() << endl;
            cerr << __AFLOW_FUNC__ << "selected_site_config_indices.size: " << selected_site_config_indices.size() << endl;
         }
         if(LDEBUG){
            for(size_t i=0; i< selected_site_config_indices.size(); i++)
            {
                cerr << __AFLOW_FUNC__ << " This is a test message" << endl;
                cerr << __AFLOW_FUNC__ << " Selected permutation configurations in each derivative superlattice: C" << selected_site_config_indices[i] << endl;
                cerr << __AFLOW_FUNC__ << " sample_config_count" <<  sample_config_count << endl;
                cerr << __AFLOW_FUNC__ << "selected_site_config_indices.size: " << selected_site_config_indices.size() << endl;
            }
         }
         std::vector<unsigned long long int> selected_indices_set;
         for(size_t i = 0; i < hnf_count; i++){
             for(size_t j : selected_site_config_indices){
                 selected_indices_set.push_back(i * types_config_permutations_count + j);
                 if(LDEBUG)
                 {
                    cerr << __AFLOW_FUNC__ << "selected_indices_set.size: " << selected_indices_set.size() << endl;
                    cerr << __AFLOW_FUNC__ << "i_j = " << i << j  << endl;
                 }
             }
         }
         if(LDEBUG){cerr << __AFLOW_FUNC__ << "size of selected_indices_set = " << selected_indices_set.size() << endl;}
         return selected_indices_set;
    }
    
    //CO20261006 - START
    //First round for SAMPLE_NUMBER and SAMPLE_HASH. FirstRandomSampling() drew the same site configurations in every
    //superlattice, from the first 100*SAMPLE_NUMBER configurations only, so later sites kept their starting decoration.
    //Here each draw is uniform over all hnf_count*types_config_permutations_count decorations (every superlattice has
    //the same count), returned as hnf_index*types_config_permutations_count+site_config_index, sorted and distinct.
    //The pool keeps the size YL20240426 chose, 100*SAMPLE_NUMBER, for the UFF comparison; a smaller space is taken whole.
    //std::mt19937_64 gives the same sequence for the same seed on every standard library; the bounded draw below
    //rejects the top of the range instead of using std::uniform_int_distribution, whose output differs between
    //libstdc++ and libc++, so a run and its check on another machine draw the same decorations.
    vector<unsigned long long int> POccCalculator::FirstRandomSamplingWholeSpace() const{
         stringstream message;
         unsigned long long int sample_number=0;
         string sample_number_string="";
         bool XHOST_POCC_SAMPLE_NUMBER=usePOccSampleNumber(sample_number_string, sample_number);
         string sample_hash_string="";
         unsigned long long int sample_hash_hnf=AUROSTD_MAX_ULLINT;
         unsigned long long int sample_hash_site_config=AUROSTD_MAX_ULLINT;
         bool XHOST_POCC_SAMPLE_HASH=usePOccSampleHash(sample_hash_string,sample_hash_hnf,sample_hash_site_config);
         if(types_config_permutations_count==0 || hnf_count==0){
            message << "No supercell decorations to sample [HNF_COUNT=" << hnf_count << ", TYPES_CONFIG_COUNT=" << types_config_permutations_count << "]";
            throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_RUNTIME_ERROR_);
         }
         if(hnf_count>AUROSTD_MAX_ULLINT/types_config_permutations_count){
            message << "Count of supercell decorations exceeds unsigned long long int [HNF_COUNT=" << hnf_count << ", TYPES_CONFIG_COUNT=" << types_config_permutations_count << "]";
            throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_VALUE_RANGE_);
         }
         const unsigned long long int decorations_count=hnf_count*types_config_permutations_count;
         std::set<unsigned long long int> selected;
         if(XHOST_POCC_SAMPLE_HASH){
            if(sample_hash_hnf>=hnf_count || sample_hash_site_config>=types_config_permutations_count){
               message << "POCC_SAMPLE_HASH out of range for this run [POCC_SAMPLE_HASH=" << sample_hash_string << ", HNF_COUNT=" << hnf_count << ", TYPES_CONFIG_COUNT=" << types_config_permutations_count << "]";
               throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
            }
            selected.insert(sample_hash_hnf*types_config_permutations_count+sample_hash_site_config);
         }
         if(XHOST_POCC_SAMPLE_NUMBER){
            const unsigned long long int pool_multiple=100;
            unsigned long long int pool_count=decorations_count;
            if(sample_number<=decorations_count/pool_multiple){pool_count=pool_multiple*sample_number;}
            if(pool_count==decorations_count){
               selected.clear();
               for(unsigned long long int i=0;i<decorations_count;i++){selected.insert(selected.end(),i);}
            }
            else{
               std::mt19937_64 rng((std::mt19937_64::result_type)DEFAULT_POCC_SAMPLE_SEED);
               const unsigned long long int threshold=(0ULL-decorations_count)%decorations_count;  //2^64 mod decorations_count: draws below it are rejected, so every remainder is equally likely
               unsigned long long int draw=0;
               const size_t target=selected.size()+pool_count;
               while(selected.size()<target){
                  draw=(unsigned long long int)rng();
                  if(draw<threshold){continue;}
                  selected.insert(draw%decorations_count);
               }
            }
         }
         return vector<unsigned long long int>(selected.begin(),selected.end());
    }
    //CO20261006 - STOP

    std::list<pocc::POccSuperCellSet> POccCalculator::SecondRandomSamplingWithRate(std::list<pocc::POccSuperCellSet> l_supercell_sets, unsigned long long int hnf_count){
        bool LDEBUG = (FALSE || _DEBUG_POCC_SAMPLE_ || XHOST.DEBUG);
        string sample_rate_string="";  //TL20260205
        usePOccSampleRate(sample_rate_string);  //TL20260205
        double sample_rate = setPOccSampleRate(sample_rate_string,1);  //TL20260205 use sample_rate_string from helper
        vector<unsigned long long int> ihnf_unique_supercell_num(hnf_count,0);
        l_supercell_sets.sort();
        for(unsigned long long int i=0;i<l_supercell_sets.size();i++) {
            std::list<pocc::POccSuperCellSet>::iterator it = l_supercell_sets.begin();
            std::advance(it,i);
            pocc::POccSuperCell pscs=(*it).getSuperCell();
            ihnf_unique_supercell_num[pscs.m_hnf_index]++; // calculate unique decoration configurations for each superlattices and saved in ihnf_unique_supercell_num
            if(LDEBUG){
               cerr << __AFLOW_FUNC__ << " hnf_index: " << pscs.m_hnf_index << endl;
               cerr << __AFLOW_FUNC__ << " hnf_index: " <<  pscs.m_hnf_index << " hnf_unique_num: " << ihnf_unique_supercell_num[pscs.m_hnf_index] << endl;

               cerr << __AFLOW_FUNC__ << " H" << pscs.m_hnf_index << "C" << pscs.m_site_config_index << endl;
            }
        }
        if(LDEBUG){
            cerr << __AFLOW_FUNC__ << " This is a test message" << endl;
            cerr << __AFLOW_FUNC__ << " This is second random seed sampling for DFT calculations" << endl;
            cerr << __AFLOW_FUNC__ << " Random seed sampling rate is: " << 100*sample_rate << "%" << endl;
        } 
        std::list<pocc::POccSuperCellSet> selected_random_sampling_supercell_sets;
        for(unsigned long long int i=0; i<hnf_count; i++){ // loop for each superlattice to generate the sample index for each superlattice
            vector<int> sample_index(ihnf_unique_supercell_num[i]); //sample_index save the unique configuration indices for each superlattice
            if(i==0){
               iota(sample_index.begin(),sample_index.end(),0); //iota create integer vector from 0 for 1st superlattice
            }else{
               unsigned long long int summation=0;
               for(unsigned long long int j=0; j<i; j++){ summation+=ihnf_unique_supercell_num[j]; }
               if(LDEBUG) {cerr << __AFLOW_FUNC__ << "summation: " << summation << endl;}
               iota(sample_index.begin(),sample_index.end(),summation); //iota create integer vector from summation for the other supperlattice exclude from 1st
            }
            int sample_seed = DEFAULT_POCC_SAMPLE_SEED;
            srand(sample_seed); //initial random seed sampling
            size_t sample_config_count = static_cast<size_t>(ihnf_unique_supercell_num[i]*sample_rate); //calculate total amount of selected configurations for each superlattice
            if(LDEBUG)
            {
               cerr << __AFLOW_FUNC__ << " ihnf_nique_supercell_num[" << i << "] = " << ihnf_unique_supercell_num[i]<< endl;
               cerr << __AFLOW_FUNC__ << " sample_config_count[" << i << "] = "  << sample_config_count << endl;
            }
            std::unordered_set<unsigned long long int> selected_config_indices_set; // unordered_set could auotmatically select unique index avoiding repetion and is cheaper than find
            unsigned long long int index; //TL20250612 Declare outside loop to avoid repeated memory allocation
            while(selected_config_indices_set.size() < sample_config_count) {
                index = rand() % ihnf_unique_supercell_num[i] ; //TL20250612 select permuation decoration configuration index
                selected_config_indices_set.insert(index);
            }
            std::vector<unsigned long long int> selected_config_indices(selected_config_indices_set.begin(), selected_config_indices_set.end());
            sort(selected_config_indices.begin(), selected_config_indices.end());
            for(size_t i=0; i< sample_config_count; i++){
                std::list<pocc::POccSuperCellSet>::iterator it = l_supercell_sets.begin();
                std::advance(it,sample_index[selected_config_indices[i]]);
                selected_random_sampling_supercell_sets.push_back(*it);//push back the selected supercell
                if(LDEBUG){
                   pocc::POccSuperCell pscs=(*it).getSuperCell();
                   cerr << __AFLOW_FUNC__ << " This is a test message" << endl;
                   cerr << __AFLOW_FUNC__ << " Second sampling is: " << "H" << pscs.m_hnf_index << "C" << pscs.m_site_config_index << endl;
                }
            }
         }
         return selected_random_sampling_supercell_sets;
    }

    std::list<pocc::POccSuperCellSet> POccCalculator::SecondRandomSampling(std::list<pocc::POccSuperCellSet> l_supercell_sets){
        bool LDEBUG = (FALSE || _DEBUG_POCC_SAMPLE_ || XHOST.DEBUG);
        std::unordered_set<unsigned long long int> selected_config_indices_set; // unordered_set could auotmatically select unique index avoiding repetion and is cheaper than find
        unsigned long long int sample_number_value=0;
        string sample_number_string="";
        bool XHOST_POCC_SAMPLE_NUMBER=usePOccSampleNumber(sample_number_string, sample_number_value);  //TL20260204
        string sample_rate_string="";  //TL20260205
        bool XHOST_POCC_SAMPLE_RATE=usePOccSampleRate(sample_rate_string);  //TL20260205
        size_t sample_number = 0; //TL20250612 Initialize to avoid uninitialized warning
        if(XHOST_POCC_SAMPLE_RATE){
           double sample_rate = setPOccSampleRate(sample_rate_string,1);  //TL20260205 use sample_rate_string from helper
           sample_number = static_cast<size_t>(l_supercell_sets.size() * sample_rate);
        } 
        if(XHOST_POCC_SAMPLE_NUMBER){sample_number = static_cast<size_t>(sample_number_value);} // calculate total amount of selected decoration permutation configurations for each derivative supperlattice

        //TL20250612 Check if sample_number is valid
        if(sample_number == 0) {
            stringstream message;
            message << "No valid sampling parameters provided. Either --pocc_sampling_rate or --pocc_sampling_number must be set.";
            throw aurostd::xerror(__AFLOW_FILE__,__AFLOW_FUNC__,message,_INPUT_ILLEGAL_);
        }

        //CO20261006 - with no more unique supercells than sample_number, keep them all (the loop below would never end)
        if(l_supercell_sets.size()<=sample_number){
            stringstream message;
            message << "Keeping all " << l_supercell_sets.size() << " unique supercells (second round of " << sample_number << ")";
            pflow::logger(__AFLOW_FILE__,__AFLOW_FUNC__,message,m_aflags,*p_FileMESSAGE,*p_oss,_LOGGER_MESSAGE_);
            return l_supercell_sets;
        }

        int sample_seed = DEFAULT_POCC_SAMPLE_SEED;
        srand(sample_seed); //initial random seed sampling
        l_supercell_sets.sort();
        unsigned long long int index; // Declare outside loop to avoid repeated memory allocation
        while(selected_config_indices_set.size() < sample_number) {
            index = rand() % l_supercell_sets.size(); //select random number from whole l_supercell_sets
            selected_config_indices_set.insert(index);
        }
        std::vector<unsigned long long int> selected_config_indices(selected_config_indices_set.begin(), selected_config_indices_set.end());
        sort(selected_config_indices.begin(), selected_config_indices.end());//set selected_indices to save sorted indices
        if(LDEBUG){cerr << __AFLOW_FUNC__ << " selected_config_indices.size: " << selected_config_indices.size() << endl;}
        std::list<pocc::POccSuperCellSet> selected_random_sampling_supercell_sets;
        for(size_t i = 0; i < sample_number; i++){
            std::list<pocc::POccSuperCellSet>::iterator it = l_supercell_sets.begin();
            std::advance(it,selected_config_indices[i]);
            selected_random_sampling_supercell_sets.push_back(*it);//push back the selected supercell
        }

         return selected_random_sampling_supercell_sets;
    }
}
#endif
