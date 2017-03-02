/*==============================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 */

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <cstdlib>

#include "fasta-file.hpp"
#include "cigar2events.hpp"

struct cFastaFile {
    CPP::IndexedFastaFile file;
    
    cFastaFile(std::string const &filepath) : file(CPP::IndexedFastaFile::load(filepath)) {}
};

extern "C" {
#include <stdlib.h>
#include "expandCIGAR.h"
    
    struct cFastaFile *loadFastaFile(unsigned const length, char const *const path)
    {
        std::string const &filepath = length ? std::string(path, path + length) : std::string(path);
        try {
            return new cFastaFile(filepath);
        }
        catch (...) {
            return nullptr;
        }
    }
    
    void unloadFastaFile(struct cFastaFile *const file)
    {
        delete file;
    }
    
    int FastaFile_getNamedSequence(struct cFastaFile *const file, unsigned const length, char const *const seqId)
    {
        std::string const &name = length ? std::string(seqId, seqId + length) : std::string(seqId);
        return file->file.find(name);
    }

    unsigned FastaFile_getSequenceData(struct cFastaFile *file, int referenceNumber, char const **sequence) {
        CPP::FastaFile::Sequence const &seq = file->file.sequences[referenceNumber];
        *sequence = seq.data;
        return seq.length;
    }
    
    int validateCIGAR(unsigned length, char const CIGAR[/* length */], unsigned *refLength, unsigned *seqLength)
    {
        std::string const &cigar = length ? std::string(CIGAR, CIGAR + length) : std::string(CIGAR);
        try {
            std::pair<unsigned, unsigned> const &lengths = CPP::measureCIGAR(cigar);
            if (refLength)
                *refLength = lengths.first;
            if (seqLength)
                *seqLength = lengths.second;
            return 0;
        }
        catch (...) {
            return -1;
        }
    }
    
    int expandCIGAR(  struct Event **const result
                    , unsigned const length
                    , char const *const CIGAR
                    , char const *const sequence
                    , unsigned const position
                    , struct cFastaFile *const file
                    , int referenceNumber)
    {
        std::string const &cigar = length ? std::string(CIGAR, CIGAR + length) : std::string(CIGAR);
        try {
            std::vector<CPP::Event> const &events = CPP::expandAlignment(file->file.sequences[referenceNumber], position, cigar, sequence);
            int const N = int(events.size());
            struct Event *const rslt = (struct Event *)malloc(N * sizeof(**result));
            for (int i = 0; i < N; ++i) {
                rslt[i].type = events[i].type;
                rslt[i].length = events[i].length;
                rslt[i].refPos = events[i].refPos;
                rslt[i].seqPos = events[i].seqPos;
            }
            *result = rslt;
            return N;
        }
        catch (...) {
            return -1;
        }
    }
}