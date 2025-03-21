import React from 'react';
import classNames from 'classnames';

type CardProps = {
    title?: string,
    childrenContainerClassName?: string,
    className?: string,
};

const Card: React.FC<CardProps> = ({ title, childrenContainerClassName = '', children, className }) => (
    <div className={className}>
        {!!title && <h1 className="mb-4 text-2xl font-medium">{title}</h1>}

        <div className={classNames(['bg-navy-lighter rounded-md p-6 ', childrenContainerClassName])}>
            {children}
        </div>
    </div>
);

export default Card;
