import React, { useState, useEffect } from 'react';
import { IconAlignRight, IconBox, IconPlane, IconSwitchHorizontal, IconUsers, IconBolt } from '@tabler/icons';
import fuselage from '../../Assets/320neo-outline-nose.svg';
import { useSimVar } from '../../../Common/simVars';
import { useAppSelector } from '../../Store/store';

export const OverviewPage = () => {
    const [, setUnitConversion] = useState(1000);

    const { units } = useAppSelector((state) => state.simbrief.data);

    useEffect(() => {
        const unitConv = (units === 'kgs') ? 1000 : 2240;
        setUnitConversion(unitConv);
    }, [units]);

    let [airline] = useSimVar('ATC AIRLINE', 'String', 1_000);

    if (airline === 0 || null || '') {
        airline = 'FlyByWire Simulations';
    }

    return (
        <div className="overflow-hidden p-6 mt-4 mr-3 w-1/2 rounded-lg border-2 shadow-md border-theme-accent h-efb">
            <h2 className="text-2xl font-medium">Airbus A320neo</h2>
            <span className="text-lg">{airline}</span>
            <div className="flex justify-center items-center mt-6">
                <img className="mr-32 -ml-96 h-48 flip-horizontal" src={fuselage} />
            </div>
            <div className="flex mt-8">
                <div className="w-1/2">
                    <h3 className="flex items-center text-xl font-medium">
                        <IconPlane className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Model
                    </h3>
                    <span className="mt-2 text-lg">A320-251N [A20N]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconSwitchHorizontal className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Range
                    </h3>
                    <span className="mt-2 text-lg">3400 [nm]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconBox className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        MRW
                    </h3>
                    <span className="mt-2 text-lg">79,400 [kg]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconBox className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        MZFW
                    </h3>
                    <span className="mt-2 text-lg">64,300 [kg]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconUsers className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Max PAX
                    </h3>
                    <span className="mt-2 text-lg">174</span>
                </div>
                <div className="w-1/2">
                    <h3 className="flex items-center text-xl font-medium">
                        <IconBolt className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Engines
                    </h3>
                    <span className="mt-2 text-lg">CFM LEAP 1A-26</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconAlignRight className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Mmo
                    </h3>
                    <span className="mt-2 text-lg">0.82</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconBox className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        MTOW
                    </h3>
                    <span className="mt-2 text-lg">79,000 [kg]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconBox className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Max Fuel Capacity
                    </h3>
                    <span className="mt-2 text-lg">23,721 [l]</span>

                    <h3 className="flex items-center mt-6 text-xl font-medium">
                        <IconBox className="mr-2" size={23} stroke={1.5} strokeLinejoin="miter" />
                        {' '}
                        Max Cargo
                    </h3>
                    <span className="mt-2 text-lg">9,435 [kg]</span>
                </div>
            </div>
        </div>
    );
};
