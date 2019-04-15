import React, { Component } from 'react';
import { connect } from 'react-redux';
// Components
import { Button } from 'react-bootstrap'
// Services and redux action
import { OrderAction } from 'actions';
import { ApiService, ApiServiceScatter } from 'services';

class DelayCancel extends Component {
    constructor(props) {
        super(props);

        this.handleClick = this.handleClick.bind(this);
    }

    handleClick(event) {
        event.preventDefault();

        const { order: { orderKey }, setOrder, user: { account }, orders: { listOrders }, scatter: { scatter } } = this.props;

        ApiServiceScatter.delayCancel(orderKey, scatter).then(() => {
            ApiService.getOrderByKey(orderKey).then((order) => {
                setOrder({ listOrders: listOrders, order: order, account });
            })
        }).catch((err) => { console.error(err) });
    }

    render() {

        let isPrint = false;

        const { order: { state, currentActor, date } } = this.props;

        if (state === "2" || state === "3" || state === "4") {
            if (currentActor === "buyer" && new Date(date).getTime() < Date.now()) {
                isPrint = true;
            }
        }

        return (
            <div>
                {isPrint &&
                    <Button
                        onClick={this.handleClick}
                        variant="danger"
                        className="float-right"
                    >
                        DELAY CANCEL
                </Button>
                }
            </div>
        )
    }
}

const mapStateToProps = state => state;

const mapDispatchToProps = {
    setOrder: OrderAction.setOrder,
};

export default connect(mapStateToProps, mapDispatchToProps)(DelayCancel);