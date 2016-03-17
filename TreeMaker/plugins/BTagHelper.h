#include "aTGCsAnalysis/TreeMaker/interface/BTagCalibrationStandalone.h"
#include "TEfficiency.h"
#include "TH2.h"
using namespace btag;
template<class T> class BTagHelper{

	BTagCalibration calib;
	float MaxBJetPt;
	TH2D *hist_eff;
	TEfficiency *eff;
	double DiscrCut;
	std::string DiscrName;
public:
	std::auto_ptr<BTagCalibrationReader>  reader;
	BTagHelper()
	{	
		//scale factors from CSV file
		edm::FileInPath CSVFile("aTGCsAnalysis/TreeMaker/data/CSVv2.csv");
		edm::FileInPath EfficiencyFile("aTGCsAnalysis/TreeMaker/data/eff_ttbar_mu.root");
		calib = BTagCalibration("CSVv2", CSVFile.fullPath());
		reader.reset(new BTagCalibrationReader(BTagEntry::OP_TIGHT, "central"));
		reader->load(calib, BTagEntry::FLAV_B, "mujets");
		MaxBJetPt = 670.;
		//get measured efficiencies
		TFile effFile(EfficiencyFile.fullPath().c_str());
		eff = (TEfficiency*)effFile.Get("BtagAnalyzer/h2_BTaggingEff_b");
		hist_eff = (TH2D*) eff->GetPassedHistogram();//get histogram to get binning info
		DiscrCut = 0.935;
		DiscrName = "pfCombinedInclusiveSecondaryVertexV2BJetTags";
	}
	double getScaleFactor(T jet){
		float jetPt = jet.pt();
      	if (jetPt>MaxBJetPt) jetPt = MaxBJetPt;
		 double jet_scalefactor = reader->eval(BTagEntry::FLAV_B, jet.eta(), jetPt);
		 return jet_scalefactor;
	}
	double getEfficiency(T jet){
		int ptBin = hist_eff ->GetXaxis() -> FindBin(jet.pt());
		int etaBin = hist_eff ->GetYaxis() -> FindBin(jet.eta());
		if (jet.pt() >= hist_eff -> GetXaxis() -> GetBinUpEdge(hist_eff -> GetNbinsX()) ) ptBin =  hist_eff -> GetNbinsX();//protection in case we go above the binning limit
		double efficiency = eff->GetEfficiency(eff->GetGlobalBin(ptBin,etaBin));
		return efficiency;
	}
	double getEventWeight(edm::Handle<edm::View<T>> jets){
		double probabMC = 1., probabData = 1.;
		for(unsigned int iBtag = 0; iBtag < jets -> size(); iBtag ++)
		{
			if (jets->at(iBtag).bDiscriminator(DiscrName) > DiscrCut){
				probabMC *= getEfficiency(jets->at(iBtag));
				probabData *= getEfficiency(jets->at(iBtag)) * getScaleFactor(jets->at(iBtag));
			}
			else {
				probabMC *= (1 - getEfficiency(jets->at(iBtag)));
				probabData *= (1 - getEfficiency(jets->at(iBtag))*getScaleFactor(jets->at(iBtag)));
			}			
		}
		double weight = probabData/probabMC;
		return weight;
	}

};