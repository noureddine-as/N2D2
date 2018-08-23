/*
    (C) Copyright 2016 CEA LIST. All Rights Reserved.
    Contributor(s): Olivier BICHLER (olivier.bichler@cea.fr)

    This software is governed by the CeCILL-C license under French law and
    abiding by the rules of distribution of free software.  You can  use,
    modify and/ or redistribute the software under the terms of the CeCILL-C
    license as circulated by CEA, CNRS and INRIA at the following URL
    "http://www.cecill.info".

    As a counterpart to the access to the source code and  rights to copy,
    modify and redistribute granted by the license, users are provided only
    with a limited warranty  and the software's author,  the holder of the
    economic rights,  and the successive licensors  have only  limited
    liability.

    The fact that you are presently reading this means that you have had
    knowledge of the CeCILL-C license and that you accept its terms.
*/

#include "Generator/MappingGenerator.hpp"

const N2D2::MappingGenerator::Mapping N2D2::MappingGenerator::defaultMapping
    = {1, // sizeX
       1, // sizeY
       1, // strideX
       1, // strideY
       0, // offsetX
       0, // offsetY
       0 // nbIterations
};

N2D2::MappingGenerator::Mapping
N2D2::MappingGenerator::getMapping(IniParser& iniConfig,
                                   const std::string& section,
                                   const std::string& name,
                                   const Mapping& defaultMapping)
{
    if (!iniConfig.currentSection(section, false))
        throw std::runtime_error("Missing [" + section + "] section.");

    Mapping mapping;

    if (iniConfig.isProperty(name + ".Size"))
        mapping.sizeX = mapping.sizeY = iniConfig.getProperty
                                        <unsigned int>(name + ".Size");
    else {
        mapping.sizeX = iniConfig.getProperty
                        <unsigned int>(name + ".SizeX", defaultMapping.sizeX);
        mapping.sizeY = iniConfig.getProperty
                        <unsigned int>(name + ".SizeY", defaultMapping.sizeY);
    }

    if (iniConfig.isProperty(name + ".Stride"))
        mapping.strideX = mapping.strideY = iniConfig.getProperty
                                            <unsigned int>(name + ".Stride");
    else {
        mapping.strideX = iniConfig.getProperty<unsigned int>(
            name + ".StrideX", defaultMapping.strideX);
        mapping.strideY = iniConfig.getProperty<unsigned int>(
            name + ".StrideY", defaultMapping.strideY);
    }

    if (iniConfig.isProperty(name + ".Offset"))
        mapping.offsetX = mapping.offsetY = iniConfig.getProperty
                                            <unsigned int>(name + ".Offset");
    else {
        mapping.offsetX = iniConfig.getProperty<unsigned int>(
            name + ".OffsetX", defaultMapping.offsetX);
        mapping.offsetY = iniConfig.getProperty<unsigned int>(
            name + ".OffsetY", defaultMapping.offsetY);
    }

    mapping.nbIterations = iniConfig.getProperty<unsigned int>(
        name + ".NbIterations", defaultMapping.nbIterations);
    return mapping;
}

N2D2::Matrix<bool> N2D2::MappingGenerator::generate(StimuliProvider& sp,
                                                    std::shared_ptr
                                                    <Cell> parent,
                                                    unsigned int nbOutputs,
                                                    IniParser& iniConfig,
                                                    const std::string& section,
                                                    const Mapping
                                                    & defaultMapping)
{
    if (!iniConfig.currentSection(section, false))
        throw std::runtime_error("Missing [" + section + "] section.");

    if (iniConfig.isProperty("Mapping.NbGroups")
        && iniConfig.isProperty("Mapping.ChannelsPerGroup"))
    {
        throw std::runtime_error(
            "Mapping.NbGroups and Mapping.ChannelsPerGroup are mutually"
            " exclusive in section [" + section + "] in network"
            " configuration file: " + iniConfig.getFileName());
    }

    const std::string parentName = (!parent) ? "env" : parent->getName();
    const unsigned int nbChannels = (!parent) ? sp.getNbChannels()
                                              : parent->getNbOutputs();

    const bool isMappingFunction = iniConfig.isProperty("Mapping.*");
    const std::string mappingPrefix = "Mapping(" + parentName + ")";
    const bool isFunction = isMappingFunction
                            || iniConfig.isProperty(mappingPrefix + ".*");

    Matrix<bool> map = Matrix<bool>(nbChannels, nbOutputs, !isFunction);

    if (iniConfig.isProperty("Mapping.NbGroups")
        || iniConfig.isProperty("Mapping.ChannelsPerGroup"))
    {
        unsigned int nbGroups;
        unsigned int nbChannelsPerGroup;

        if (iniConfig.isProperty("Mapping.NbGroups")) {
            nbGroups = iniConfig.getProperty<unsigned int>("Mapping.NbGroups");
            nbChannelsPerGroup = nbChannels / nbGroups;
        }
        else {
            nbChannelsPerGroup = iniConfig.getProperty
                                    <unsigned int>("Mapping.ChannelsPerGroup");
            nbGroups = nbChannels / nbChannelsPerGroup;
        }

        if (nbChannels % nbGroups != 0) {
            throw std::runtime_error(
                "Mapping.NbGroups must be a multiple of the number of input"
                " channels in section [" + section + "] in network"
                " configuration file: " + iniConfig.getFileName());
        }

        size_t outputGroupOffset = 0;
        size_t channelGroupOffset = 0;

        for (size_t group = 0; group < nbGroups; ++group) {
            const size_t outputGroupSize = (nbOutputs - outputGroupOffset)
                                                / (nbGroups - group);

            for (size_t output = outputGroupOffset;
                output < outputGroupOffset + outputGroupSize; ++output)
            {
                for (size_t channel = channelGroupOffset;
                    channel < channelGroupOffset + nbChannelsPerGroup;
                    ++channel)
                {
                    map(channel, output) = true;
                }
            }

            outputGroupOffset += outputGroupSize;
            channelGroupOffset += nbChannelsPerGroup;
        }
    }
    else {
        if (iniConfig.isProperty("Map(" + parentName + ")")) {
            // Hand-made mapping matrix
            map << iniConfig.getProperty<std::string>("Map(" + parentName + ")");
        }
        else if (isFunction && !map.empty()) {
            // Mapping function
            Mapping mapping
                = getMapping(iniConfig, section, mappingPrefix, defaultMapping);

            if (mapping.nbIterations == 0)
                mapping.nbIterations = std::max(nbOutputs, nbChannels);

            for (unsigned int x = mapping.offsetX, y = mapping.offsetY, i = 0;
                 i < mapping.nbIterations;
                 x += mapping.strideX, y += mapping.strideY, ++i) {
                for (unsigned int patchX = 0; patchX < mapping.sizeX; ++patchX)
                {
                    for (unsigned int patchY = 0; patchY < mapping.sizeY;
                        ++patchY)
                    {
                        map((y + patchY) % nbChannels,
                            (x + patchX) % nbOutputs) = true;
                    }
                }
            }
        }
    }

    return map;
}
