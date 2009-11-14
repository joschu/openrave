// Copyright (C) 2006-2008 Rosen Diankov (rdiankov@cs.cmu.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#ifndef OPENRAVE_LOGGING_H
#define OPENRAVE_LOGGING_H

/// used to log scene elements
class LoggingProblem : public ProblemInstance
{
    inline boost::shared_ptr<LoggingProblem> shared_problem() { return boost::static_pointer_cast<LoggingProblem>(shared_from_this()); }
    inline boost::shared_ptr<LoggingProblem const> shared_problem_const() const { return boost::static_pointer_cast<LoggingProblem const>(shared_from_this()); }

 public:
 LoggingProblem(EnvironmentBasePtr penv) : ProblemInstance(penv)
    {
        bDestroyThread = false;
        bAbsoluteTiem = false;
        bDoLog = false;
    }

    virtual ~LoggingProblem() {}
    virtual void Destroy()
    {
        bDestroyThread = true;
        bDoLog = false;
        bAbsoluteTiem = false;
        if( !!_threadlog )
            _threadlog->join();
        _threadlog.reset();
    }

    virtual int main(const string& cmd)
    {
        Destroy();
        __mapCommands.clear();
        RegisterCommand("savescene",boost::bind(&LoggingProblem::SaveScene,shared_problem(),_1,_2),
                        "Saves the entire scene in an xml file. If paths are relative,\n"
                        "should only be opened from the dirctory openrave was launched in\n"
                        "Usage: [filename %s] [absolute (default is relative)]");
        RegisterCommand("startreplay",boost::bind(&LoggingProblem::StartReplay,shared_problem(),_1,_2),
                        "Starts replaying a recording given a speed (can be negative).\n"
                        "Usage: [speed %f] [filename %s]");
        RegisterCommand("startrecording",boost::bind(&LoggingProblem::StartRecording,shared_problem(),_1,_2),
                        "Starts recording the scene given a realtime delta.\n"
                        "If a current recording is in progress, stop it.\n"
                        "Usage: [filename %s] [realtime %f]");
        RegisterCommand("stoprecording",boost::bind(&LoggingProblem::StopRecording,shared_problem(),_1,_2),
                        "Stop recording.\n"
                        "Usage: [speed %f] [filename %s]");
        RegisterCommand("help",boost::bind(&LoggingProblem::Help,shared_problem(),_1,_2), "display this message.");

        bDestroyThread = false;
        _threadlog.reset(new boost::thread(boost::bind(&LoggingProblem::_log_thread,shared_problem())));
        return 0;
    }
    
    /// returns true when problem is solved
    virtual bool SimulationStep(dReal fElapsedTime)
    {
        return false;
    }
    
 private:
    virtual bool SaveScene(ostream& sout, istream& sinput)
    {
        EnvironmentMutex::scoped_lock lock(GetEnv()->GetMutex());

        string cmd, filename;
        bool bAbsolutePaths = false;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput )
                break;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "filename" )
                sinput >> filename;
            else if( cmd == "absolute" )
                bAbsolutePaths = true;
            else break;

            if( !sinput ) {
                RAVELOG_ERRORA("failed\n");
                break;
            }
        }

        if( filename.size() == 0 ) {
            RAVELOG_ERRORA("bad filename\n");
            sout << "0" << endl;
            return false;
        }

        ofstream fout(filename.c_str());
        if( !fout ) {
            RAVELOG_ERRORA("failed to open file %s\n", filename.c_str());
            sout << "0" << endl;
            return false;
        }

        fout << "<Environment>" << endl;

        int tabwidth = 2;
        TransformMatrix t = TransformMatrix(GetEnv()->GetCameraTransform());
        fout << setw(tabwidth) << " "
             << "<camtrans>" << t.trans.x << " " << t.trans.y << " " << t.trans.z << "</camtrans>" << endl;
        fout << setw(tabwidth) << " " << "<camrotmat>";
        for(int i = 0; i < 3; ++i)
            fout << t.m[4*i+0] << " " << t.m[4*i+1] << " " << t.m[4*i+2] << " ";
        fout << "</camrotmat>" << endl << endl;

        vector<KinBodyPtr> vecbodies;
        GetEnv()->GetBodies(vecbodies);
        FOREACHC(itbody, vecbodies) {
            KinBodyPtr pbody = *itbody;

            if( pbody->GetXMLFilename().size() > 0 ) {
                fout << setw(tabwidth) << " "
                     << (pbody->IsRobot()?"<Robot":"<KinBody") << " name=\"" << pbody->GetName() << "\"";
                fout << " file=\"" << pbody->GetXMLFilename() << "\">" << endl;
            }
            else {
                // have to process each link/geometry file
                RAVELOG_ERRORA(str(boost::format("cannot process object %s defined inside environment file\n")%pbody->GetName()));
                continue;
                //fout << ">" << endl;
            }
        
            t = pbody->GetTransform();
            fout << setw(tabwidth*2) << " "
                 << "<Translation>" << t.trans.x << " " << t.trans.y << " " << t.trans.z << "</Translation>" << endl;
            fout << setw(tabwidth*2) << " " << "<rotationmat>";
            for(int i = 0; i < 3; ++i)
                fout << t.m[4*i+0] << " " << t.m[4*i+1] << " " << t.m[4*i+2] << " ";
            fout << "</rotationmat>" << endl << endl;

            fout << setw(tabwidth*2) << " " << "<jointvalues>";

            vector<dReal> values;
            pbody->GetJointValues(values);
            FOREACH(it, values)
                fout << *it << " ";
            fout << "</jointvalues>" << endl;
            fout << setw(tabwidth) << " ";
            fout << (pbody->IsRobot()?"</Robot>":"</KinBody>") << endl;
        }
    
        fout << "</Environment>" << endl;
        sout << "1" << endl;

        return true;
    }

    virtual bool StartReplay(ostream& sout, istream& sinput)
    {
        RAVELOG_ERRORA("not implemented\n");
        return false;
    }
    virtual bool StartRecording(ostream& sout, istream& sinput)
    {
        RAVELOG_ERRORA("not implemented\n");
        return false;
    }
    virtual bool StopRecording(ostream& sout, istream& sinput)
    {
        RAVELOG_ERRORA("not implemented\n");
        return false;
    }

    virtual bool Help(ostream& sout, istream& sinput)
    {
        sout << "----------------------------------" << endl
             << "Logging Problem Commands:" << endl;
        GetCommandHelp(sout);
        sout << "----------------------------------" << endl;
        return true;
    }

    void _log_thread()
    {
        while(!bDestroyThread) {
            // record
            //        if( pfLog == NULL ) {
            //            pfLog = fopen("motion.txt", "wb");
            //        }
            //        
            //        fTotalTime += fElapsedTime;
            //        if( pfLog != NULL && (fTotalTime-fLogTime) > 0.05f ) {
            //            
            //            fwrite(&fTotalTime, sizeof(float), 1, pfLog);
            //            
            //            int numobjs = (int)GetEnv()->GetBodies().size();
            //            fwrite(&numobjs, 4, 1, pfLog);
            //            
            //            Transform t;
            //            vector<dReal> vjoints;
            //            std::vector<KinBody*>::const_iterator itbody;
            //            FORIT(itbody, GetEnv()->GetBodies()) {
            //                size_t len = wcslen((*itbody)->GetName());
            //                fwrite(&len, 4, 1, pfLog);
            //                fwrite((*itbody)->GetName(), len*sizeof((*itbody)->GetName()[0]), 1, pfLog);
            //                
            //                t = (*itbody)->GetTransform();
            //                fwrite(&t, sizeof(t), 1, pfLog);
            //                
            //                (*itbody)->GetJointValues(vjoints);
            //                
            //                len = vjoints.size();
            //                fwrite(&len, 4, 1, pfLog);
            //                if( len > 0 )
            //                    fwrite(&vjoints[0], sizeof(vjoints[0]) * vjoints.size(), 1, pfLog);
            //            }
            //            
            //            fLogTime = fTotalTime;
            //        }
            Sleep(1);
        }
    }

    boost::shared_ptr<boost::thread> _threadlog;
    bool bDoLog, bDestroyThread, bAbsoluteTiem;
};

#endif
