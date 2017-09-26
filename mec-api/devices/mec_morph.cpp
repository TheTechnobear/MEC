#include "mec_morph.h"
#include "../mec_log.h"
#include "morph/panels/Panel.h"
#include "morph/Instrument.h"
#include "morph/panels/CompositePanel.h"
#include "morph/overlays/OverlayFactory.h"
#include "morph/panels/SinglePanel.h"

#include <src/sensel.h>

#include <vector>
#include <memory>
#include <unordered_set>
#include <cmath>
#include <set>
#include <map>

namespace mec {
namespace morph {

class Impl {
public:
    Impl(ISurfaceCallback &surfaceCallback, ICallback &callback) :
            surfaceCallback_(surfaceCallback), callback_(callback), active_(false) {}

    ~Impl() {}

    bool init(void *preferences) {
        Preferences prefs(preferences);

        if (active_) {
            LOG_2("morph::Impl::init - already active, calling deinit");
            deinit();
        }

        int numPanels = initPanels(prefs);
        if(numPanels < 1) {
            LOG_0("morph::Impl::init - umable to initialize panels, exiting");
            return false;
        }

        bool compositePanelsInitialized = initCompositePanels(prefs);
        if(!compositePanelsInitialized) {
            LOG_0("morph::Impl::init - umable to initialize composite panels, exiting");
            return false;
        }

        bool instrumentsInitialized = initInstruments(prefs);
        if(!instrumentsInitialized) {
            LOG_0("morph::Impl::init - umable to initialize instruments, exiting");
            return false;
        }

        LOG_1("Morph - " << numPanels << " panels initialized");

        active_ = true;
        return active_;
    }

    void deinit() {
        panels_.clear();
        LOG_1("Morph - panels freed");
    }

    bool isActive() {
        return active_;
    }

    bool process() {
        bool allProcessingSucceeded = true;
        for(auto instrumentIter = instruments_.begin(); instrumentIter != instruments_.end(); ++instrumentIter) {
            bool processSucceeded = (*instrumentIter)->process();
            if(!processSucceeded) {
                LOG_0("unable to process touches for instrument " << (*instrumentIter)->getName());
                allProcessingSucceeded = false;
            }
        }
        return allProcessingSucceeded;
    }

private:
    ICallback &callback_;
    ISurfaceCallback &surfaceCallback_;
    bool active_;
    std::unordered_set<std::shared_ptr<Panel>> panels_;
    std::unordered_set<std::unique_ptr<Instrument>> instruments_;

    int initPanels(Preferences& preferences) {
        Preferences panelsPrefs(preferences.getSubTree("panels"));
        if (!panelsPrefs.valid()) {
            LOG_0("morph::Impl::initPanels - no panels defined, in preferences, add 'panels' section.");
            return 0;
        }

        std::vector<std::string> remainingPanelNames = panelsPrefs.getKeys();

        SenselDeviceList device_list;

        SenselStatus deviceStatus = senselGetDeviceList(&device_list);
        if (deviceStatus != SENSEL_OK || device_list.num_devices == 0) {
            LOG_0("morph::Impl::initPanels - unable to detect Sensel Morph panel(s), calling deinit");
            deinit();
            return 0;
        }

        int numPanels = 0;
        for (int i = 0; i < device_list.num_devices; ++i) {
            bool deviceFound = false;
            // we run four matching passes.
            // 0) First we look for panel entries where both serial number and overlayId match,
            // 1) then for entries with at least the serial number (and no overlay specified),
            // 2) then for entries with at least the overlayId (and no serial number specified),
            // 3) then for entries that have neither serial number nor overlayId specified
            for(int matchingPass = 0; matchingPass < 4 && !deviceFound; ++matchingPass) {
                for (auto panelNameIter = remainingPanelNames.begin(); panelNameIter != remainingPanelNames.end() && !deviceFound;) {
                    Preferences panelPrefs(panelsPrefs.getSubTree(*panelNameIter));
                    const std::string &serial = panelPrefs.getString("serial", "NO_SERIAL");
                    if (matchingPass > 1 || std::strcmp(serial.c_str(), (char *) device_list.devices[i].serial_num) == 0) {
                        const std::string &overlayId = panelPrefs.getString("overlayId", "NO_OVERLAY_ID");
                        //if(matchingPass > 0 || std::strcmp(overlayId.c_str(), (char*) device_list.devices[i].overlay_id) == 0) {} -- overlay id not retrievable yet via public API, always match for the moment
                        {
                            initPanel(device_list.devices[i], *panelNameIter);
                            panelNameIter = remainingPanelNames.erase(panelNameIter);
                            ++numPanels;
                            deviceFound = true;
                            break;
                        }
                    }
                    ++panelNameIter;
                }
            }
        }
        return numPanels;
    }

    bool initCompositePanels(Preferences &preferences) {
        Preferences compositePanelsPrefs(preferences.getSubTree("composite panels"));
        if (!compositePanelsPrefs.valid()) {
            LOG_2("morph::Impl::initCompositePanels - no composite panels defined in preferences.");
            return true;
        }

        const std::vector<std::string> &compositePanelNames = compositePanelsPrefs.getKeys();

        for(auto compositePanelNameIter = compositePanelNames.begin(); compositePanelNameIter != compositePanelNames.end(); ++compositePanelNameIter) {
            const std::string &compositePanelName = *compositePanelNameIter;
            Preferences compositePanelPrefs(compositePanelsPrefs.getSubTree(compositePanelName));
            if (!compositePanelPrefs.valid()) {
                LOG_2("morph::Impl::initCompositePanels - cannot read composite panel definition for " << compositePanelName);
                continue;
            }
            void* subPanelNames = compositePanelPrefs.getArray("panels");
            if(subPanelNames == nullptr) {
                LOG_0("morph::Impl::initCompositePanels - composite panel definition '" << compositePanelName << "' is missing the 'panels' list");
                continue; //perhaps we find other valid composite panel definitions
            }
            std::vector<std::shared_ptr<Panel>> panels;
            for (int i = 0; i < compositePanelPrefs.getArraySize(subPanelNames); i++) {
                SurfaceID panelID = compositePanelPrefs.getArrayString(subPanelNames, i, "");
                for(auto panelIter = panels_.begin(); panelIter != panels_.end(); ++panelIter) {
                    if((*panelIter)->getSurfaceID() == panelID) {
                        panels.push_back(*panelIter);
                        break;
                    }
                }
            }
            std::string transition_area_properyname = "transitionAreaWidth";
            int transitionAreaWidth;
            if(compositePanelPrefs.exists(transition_area_properyname)) {
                transitionAreaWidth = compositePanelPrefs.getInt(transition_area_properyname);
            } else {
                transitionAreaWidth = 20;
                LOG_1("morph::Impl::init - property transition_area_width not defined, using default 20");
            }
            std::string maxInterpolationSteps_properyname = "maxInterpolationSteps";
            int maxInterpolationSteps;
            if(compositePanelPrefs.exists(maxInterpolationSteps_properyname)) {
                maxInterpolationSteps = compositePanelPrefs.getInt(maxInterpolationSteps_properyname);
            } else {
                maxInterpolationSteps = 10;
                LOG_1("morph::Impl::init - property maxInterpolationSteps not defined, using default 10");
            }
            std::shared_ptr<Panel> compositePanel;
            compositePanel.reset(new CompositePanel(*compositePanelNameIter, panels, transitionAreaWidth, maxInterpolationSteps));
            bool initSuccessful = compositePanel->init();
            if (initSuccessful) {
                panels_.insert(compositePanel);
            } else {
                LOG_0("morph::Impl::initCompositePanels - unable to initialize panel " << *compositePanelNameIter);
            }
        }
        return true;
    }

    bool initInstruments(Preferences &preferences) {
        Preferences instrumentsPrefs(preferences.getSubTree("instruments"));
        if (!instrumentsPrefs.valid()) {
            LOG_0("morph::Impl::initInstruments - no instruments defined in preferences, add 'instruments' subsection.");
            return false;
        }

        const std::vector<std::string> &instrumentNames = instrumentsPrefs.getKeys();
        for(auto instrumentNameIter = instrumentNames.begin(); instrumentNameIter != instrumentNames.end(); ++instrumentNameIter) {
            Preferences instrumentPrefs(instrumentsPrefs.getSubTree(*instrumentNameIter));
            if(!instrumentPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to read preferences for instrument " << *instrumentNameIter);
                continue;
            }
            SurfaceID panelID;
            if(instrumentPrefs.exists("panel")) {
                panelID = instrumentPrefs.getString("panel");
            } else {
                LOG_0("morph::Impl::initInstruments - panel not defined, add 'panel' mapping to valid panel id");
                continue;
            }
            std::shared_ptr<Panel> panel;
            for(auto panelIter = panels_.begin(); panelIter != panels_.end(); ++panelIter) {
                if((*panelIter)->getSurfaceID() == panelID) {
                    panel = *panelIter;
                    break;
                }
            }
            if(!panel) {
                LOG_0("morph::Impl::initInstruments - unable to find panel with id " << panelID);
                continue;
            }

            Preferences overlayPrefs(instrumentPrefs.getSubTree("overlay"));
            if(!overlayPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to find overlay preferences for instrument '" << *instrumentNameIter << "'");
                continue;
            }
            std::string overlayName;
            if(overlayPrefs.exists("name")) {
                overlayName = overlayPrefs.getString("name");
            } else {
                LOG_0("morph::Impl::initInstruments - unable to find overlay name preferences for instrument '" << *instrumentNameIter
                                                                                                               << "'. add 'name' entry to overlay section");
                continue;
            }

            Preferences overlayParamPrefs(overlayPrefs.getSubTree("params"));
            if(!overlayParamPrefs.valid()) {
                LOG_0("morph::Impl::initInstruments - unable to find overlay parameter preferences for instrument '" << *instrumentNameIter
                << "', add 'param' section to overlay section");
                continue;
            }
            std::unique_ptr<OverlayFunction> overlayFunction;
            overlayFunction = std::move(OverlayFactory::create(overlayName, surfaceCallback_, callback_));
            overlayFunction->init(overlayParamPrefs);
            std::unique_ptr<Instrument> instrument;
            instrument.reset(new Instrument(*instrumentNameIter, panel, std::move(overlayFunction)));
            instruments_.insert(std::move(instrument));
        }
        return true;
    }

    void initPanel(const SenselDeviceID &deviceID, const std::string& panelName) {
        LOG_1("morph::Impl::init - device id for panel " << panelName << ": (idx:" << +deviceID.idx
                                                         << ",com_port:" << deviceID.com_port
                                                         << ",serial_number:" << deviceID.serial_num << ")");
        std::shared_ptr<Panel> panel;
        panel.reset(new SinglePanel(deviceID, panelName));
        bool initSuccessful = panel->init();
        if (initSuccessful) {
            panels_.insert(panel);
        } else {
            LOG_0("morph::Impl::init - unable to initialize panel " << panelName);
        }
    }
};

} // namespace morph

Morph::Morph(ISurfaceCallback &surfaceCallback, ICallback &callback) :
    morphImpl_(new morph::Impl(surfaceCallback, callback)) {}

Morph::~Morph() {}

bool Morph::init(void *preferences) {
    return morphImpl_->init(preferences);
}

bool Morph::process() {
    return morphImpl_->process();
}

void Morph::deinit() {
    morphImpl_->deinit();
}

bool Morph::isActive() {
    return morphImpl_->isActive();
}

} // namespace mec