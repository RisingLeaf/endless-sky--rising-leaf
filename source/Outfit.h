/* Outfit.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "Dictionary.h"
#include "Paragraphs.h"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Body;
class ConditionsStore;
class DataNode;
class Effect;
class Sound;
class Sprite;
class Weapon;


// Class representing an outfit that can be installed in a ship. A ship's
// "attributes" are simply stored as a series of key-value pairs, and an outfit
// can add to or subtract from any of those values. Weapons also have another
// set of attributes unique to them, and outfits can also specify additional
// information like the sprite to use in the outfitter panel for selling them,
// or the sprite or sound to be used for an engine flare.
class Outfit
{
public:
  // These are all the possible category strings for outfits.
  static const std::vector<std::string> CATEGORIES;

  static constexpr double DEFAULT_HYPERDRIVE_COST  = 100.;
  static constexpr double DEFAULT_SCRAM_DRIVE_COST = 150.;
  static constexpr double DEFAULT_JUMP_DRIVE_COST  = 200.;

#define ADD_SPECIAL_DAMAGE(x)                                                                                          \
  #x " energy", #x " fuel", #x " heat", #x " ion", #x " scramble", #x " disruption", #x " slowing", #x " discharge",   \
      #x " corrosion", #x " leakage", #x " burn"

  static constexpr std::array<std::string_view, 172> AttributeNameTable = {
      "cargo space",
      "outfit space",
      "weapon capacity",
      "engine capacity",

      "automaton",
      "required crew",

      "drag",
      "drag reduction",
      "inertia reduction",

      "atrocity",
      "illegal",
      "installable",

      "shields",
      "shield generation",
      "shield energy",
      "shield heat",
      "shield fuel",
      "shield delay",
      "depleted shield delay",
      "delayed shield generation",
      "delayed shield energy",
      "delayed shield heat",
      "delayed shield fuel",
      "high shield permeability",
      "low shield permeability",
      "cloaked shield permeability",

      "hull",
      "hull repair rate",
      "hull energy",
      "hull heat",
      "hull fuel",

      "repair delay",
      "disabled repair delay",
      "delayed hull repair rate",
      "delayed hull energy",
      "delayed hull heat",
      "delayed hull fuel",

      "absolute threshold",
      "threshold percentage",
      "hull threshold",

      "disabled recovery time",
      "disabled recovery energy",
      "disabled recovery fuel",
      "disabled recovery heat",
      "disabled recovery ionization",
      "disabled recovery scrambling",
      "disabled recovery disruption",
      "disabled recovery slowing",
      "disabled recovery discharge",
      "disabled recovery corrosion",
      "disabled recovery leak",
      "disabled recovery burning",

      "shield generation multiplier",
      "shield energy multiplier",
      "shield heat multiplier",
      "shield fuel multiplier",
      "shield multiplier",

      "hull repair multiplier",
      "hull energy multiplier",
      "hull heat multiplier",
      "hull fuel multiplier",
      "hull multiplier",

      "cloaked regen multiplier",
      "cloaked repair multiplier",

      "turret turn multiplier",

      "energy capacity",
      "energy generation",
      "energy consumption",
      "heat generation",

      "ramscoop",
      "solar collection",
      "solar heat",

      "fuel capacity",
      "fuel consumption",
      "fuel energy",
      "fuel heat",
      "fuel generation",

      "thrust",
      "thrusting shields",
      "thrusting hull",
      ADD_SPECIAL_DAMAGE(thrusting),
      "turn",
      "turning shields",
      "turning hull",
      ADD_SPECIAL_DAMAGE(turning),
      "reverse thrust",
      "reverse thrusting shields",
      "reverse thrusting hull",
      ADD_SPECIAL_DAMAGE(reverse thrusting),
      "afterburner thrust",
      "afterburner shields",
      "afterburner hull",
      ADD_SPECIAL_DAMAGE(afterburner),
      "acceleration multiplier",
      "turn multiplier",

      "cooling",
      "active cooling",
      "cooling energy",
      "cooling inefficiency",

      "heat dissipation",
      "heat capacity",
      "overheat damage rate",
      "overheat damage threshold",

      "cloak",
      "cloak by mass",
      "cloak phasing",
      "cloak hull threshold",
      "cloaking fuel",
      "cloaking energy",
      "cloaking hull",
      "cloaking shields",
      "cloaking heat",
      "cloaking shield delay",
      "cloaking repair delay",

      "piercing protection",
      "piercing resistance",
      "shield protection",
      "cloak shield protection",
      "hull protection",
      "cloak hull protection",
      "energy protection",
      "heat protection",
      "fuel protection",

      "discharge protection",
      "corrosion protection",
      "ion protection",
      "burn protection",
      "leak protection",

      "slowing protection",
      "scramble protection",
      "disruption protection",

      "force protection",
  };

#undef ADD_SPECIAL_DAMAGE

  static consteval size_t INTERNAL_ATTR(const std::string_view name)
  {
    for(size_t i = 0; i < AttributeNameTable.size(); ++i)
      if(AttributeNameTable[i] == name) return i;
    std::unreachable();
  }

  static std::optional<size_t> InternalName(const std::string_view name)
  {
    for(size_t i = 0; i < AttributeNameTable.size(); ++i)
      if(AttributeNameTable[i] == name) return i;
    return std::nullopt;
  }


public:
  // An "outfit" can be loaded from an "outfit" node or from a ship's
  // "attributes" node.
  void Load(const DataNode &node, const ConditionsStore *playerConditions);
  bool IsDefined() const;

  [[nodiscard]] const std::string &TrueName() const;
  void                             SetTrueName(const std::string &name);
  [[nodiscard]] const std::string &DisplayName() const;
  [[nodiscard]] const std::string &PluralName() const;
  [[nodiscard]] const std::string &Category() const;
  [[nodiscard]] const std::string &Series() const;
  [[nodiscard]] int                Index() const;
  [[nodiscard]] std::string        Description() const;
  [[nodiscard]] int64_t            Cost() const;
  [[nodiscard]] double             Mass() const;
  // Get the licenses needed to buy or operate this ship.
  [[nodiscard]] const std::vector<std::string> &Licenses() const;
  // Get the image to display in the outfitter when buying this item.
  [[nodiscard]] const Sprite *Thumbnail() const;

  [[nodiscard]] constexpr const double &GetIndexed(const size_t attribute) const
  {
    return QuickLoadDictionary[attribute];
  }
  [[nodiscard]] double                                      Get(std::string_view attribute) const;
  [[nodiscard]] std::set<std::string>                       GetPositiveAttributes() const;
  [[nodiscard]] std::vector<std::pair<std::string, double>> Attributes() const;

  // Determine whether the given number of instances of the given outfit can
  // be added to a ship with the attributes represented by this instance. If
  // not, return the maximum number that can be added.
  [[nodiscard]] int CanAdd(const Outfit &other, int count = 1) const;
  // For tracking a combination of outfits in a ship: add the given number of
  // instances of the given outfit to this outfit.
  void Add(const Outfit &other, int count = 1);
  // Add the licenses required by the given outfit to this outfit.
  void AddLicenses(const Outfit &outfit);
  // Modify this outfit's attributes. Note that this cannot be used to change
  // special attributes, like cost and mass.
  void Set(std::string_view attribute, double value);

  [[nodiscard]] const std::shared_ptr<const Weapon> &GetWeapon() const;
  // Get the ammo if this is an ammo storage outfit.
  [[nodiscard]] const Outfit *AmmoStored() const;
  // Get the ammo used if this is a weapon, or stored ammo if this is a storage.
  [[nodiscard]] const Outfit *AmmoStoredOrUsed() const;

  // Get this outfit's engine flare sprites, if any.
  [[nodiscard]] const std::vector<std::pair<Body, int>> &FlareSprites() const;
  [[nodiscard]] const std::vector<std::pair<Body, int>> &ReverseFlareSprites() const;
  [[nodiscard]] const std::vector<std::pair<Body, int>> &SteeringFlareSprites() const;
  [[nodiscard]] const std::map<const Sound *, int>      &FlareSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>      &ReverseFlareSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>      &SteeringFlareSounds() const;
  // Get the afterburner effect, if any.
  [[nodiscard]] const std::map<const Effect *, int> &AfterburnerEffects() const;
  // Get this outfit's jump effects and sounds, if any.
  [[nodiscard]] const std::map<const Effect *, int> &JumpEffects() const;
  [[nodiscard]] const std::map<const Sound *, int>  &HyperSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>  &HyperInSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>  &HyperOutSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>  &JumpSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>  &JumpInSounds() const;
  [[nodiscard]] const std::map<const Sound *, int>  &JumpOutSounds() const;
  // Get this outfit's scan sounds, if any.
  [[nodiscard]] const std::map<const Sound *, int> &CargoScanSounds() const;
  [[nodiscard]] const std::map<const Sound *, int> &OutfitScanSounds() const;
  // Get the sprite this outfit uses when dumped into space.
  [[nodiscard]] const Sprite *FlotsamSprite() const;


private:
  // Add the license with the given name to the licenses required by this outfit, if it is not already present.
  void AddLicense(const std::string &name);


private:
  bool        isDefined = false;
  std::string trueName;
  std::string displayName;
  std::string pluralName;
  std::string category;
  // The series that this outfit is a part of and its index within that series.
  // Used for sorting within shops.
  std::string   series;
  int           index = 0;
  Paragraphs    description;
  const Sprite *thumbnail = nullptr;
  int64_t       cost      = 0;
  double        mass      = 0.;
  // Licenses needed to purchase this item.
  std::vector<std::string> licenses;

  std::array<double, AttributeNameTable.size()> QuickLoadDictionary{};
  Dictionary                                    attributes;

  std::shared_ptr<const Weapon> weapon;
  // Non-weapon outfits can have ammo so that storage outfits
  // properly remove excess ammo when the storage is sold, instead
  // of blocking the sale of the outfit until the ammo is sold first.
  const Outfit *ammoStored = nullptr;

  // The integers in these pairs/maps indicate the number of
  // sprites/effects/sounds to be placed/played.
  std::vector<std::pair<Body, int>> flareSprites;
  std::vector<std::pair<Body, int>> reverseFlareSprites;
  std::vector<std::pair<Body, int>> steeringFlareSprites;
  std::map<const Sound *, int>      flareSounds;
  std::map<const Sound *, int>      reverseFlareSounds;
  std::map<const Sound *, int>      steeringFlareSounds;
  std::map<const Effect *, int>     afterburnerEffects;
  std::map<const Effect *, int>     jumpEffects;
  std::map<const Sound *, int>      hyperSounds;
  std::map<const Sound *, int>      hyperInSounds;
  std::map<const Sound *, int>      hyperOutSounds;
  std::map<const Sound *, int>      jumpSounds;
  std::map<const Sound *, int>      jumpInSounds;
  std::map<const Sound *, int>      jumpOutSounds;
  std::map<const Sound *, int>      cargoScanSounds;
  std::map<const Sound *, int>      outfitScanSounds;
  const Sprite                     *flotsamSprite = nullptr;
};


// These get called a lot, so inline them for speed.
inline int64_t                              Outfit::Cost() const { return cost; }
inline double                               Outfit::Mass() const { return mass; }
inline const std::shared_ptr<const Weapon> &Outfit::GetWeapon() const { return weapon; }
