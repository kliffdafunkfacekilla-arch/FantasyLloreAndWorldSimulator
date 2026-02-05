#include "../../include/SimulationModules.hpp"
#include <cstdlib>
#include <string>
#include <vector>


namespace NameGenerator {

const std::vector<std::string> prefixes = {
    "Ael", "Bor", "Cym", "Dra", "Eld", "Fae", "Gor", "Hel", "Ith", "Kaa",
    "Lor", "Mor", "Nyr", "Oth", "Pyr", "Qel", "Ryn", "Syl", "Tor", "Ulf",
    "Vyr", "Wyl", "Xar", "Yth", "Zor", "Aur", "Ben", "Cra", "Del", "Eri"};

const std::vector<std::string> middles = {"an", "ar", "en", "ir", "on",
                                          "ur", "a",  "e",  "i",  "o",
                                          "u",  "th", "ld", "rn"};

const std::vector<std::string> suffixes = {
    "gard", "ia",   "th",   "dor",  "mar",  "wen",  "vir",
    "bus",  "don",  "ius",  "land", "heim", "grad", "vale",
    "port", "keep", "stad", "wall", "burg", "ton"};

const std::vector<std::string> titles = {
    "Kingdom", "Empire",  "Republic", "Dominion", "Union", "Horde",
    "Expanse", "Enclave", "Reach",    "March",    "Realm", "Dominion"};

std::string GeneratePersonName() {
  std::string name = prefixes[rand() % prefixes.size()];
  if (rand() % 2 == 0)
    name += middles[rand() % middles.size()];
  name += suffixes[rand() % suffixes.size()];
  return name;
}

std::string GenerateFactionName() {
  std::string name = "The " + prefixes[rand() % prefixes.size()];
  name += suffixes[rand() % suffixes.size()];
  name += " " + titles[rand() % titles.size()];
  return name;
}

std::string GenerateCityName() {
  std::string name = prefixes[rand() % prefixes.size()];
  if (rand() % 3 == 0)
    name += middles[rand() % middles.size()];
  name += suffixes[rand() % suffixes.size()];
  return name;
}

} // namespace NameGenerator
