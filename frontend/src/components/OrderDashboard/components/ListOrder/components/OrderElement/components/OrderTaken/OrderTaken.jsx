import React, { Component } from 'react';
import { connect } from 'react-redux';
// Components
import { Button, InputGroup, FormControl } from 'react-bootstrap'
// Services and redux action
import { OrderAction } from 'actions';
import { ApiService, ApiServiceScatter } from 'services';
import ReadQRCode from '../ReadQRCode';

class OrderTaken extends Component {
    constructor(props) {
        super(props);

        this.state = {
            key: ""
        }
    }

    handleChange = event => {
        const { value } = event.target;

        this.setState({
            key: value,
        });
    }

    handleReadQRCode = key => {
        this.setState({
            key: key
        })
    }

    handleClick = event => {
        event.preventDefault();

        const { key } = this.state;
        const { setOrder, order: { orderKey }, orders: { listOrders }, scatter: { scatter } } = this.props;
        const accountScatter = scatter.identity.accounts.find(x => x.blockchain === 'eos');

        return ApiServiceScatter.orderTaken(orderKey, key, scatter).then(() => {
            ApiService.getOrderByKey(orderKey).then((order) => {
                setOrder({ listOrders: listOrders, order: order, account: accountScatter.name });
            })
        }).catch((err) => { console.error(err) });
    }

    render() {

        const { order } = this.props;
        const { key } = this.state;

        let isPrint = false;

        if (order.state === "3" && order.currentActor === "deliver") {
            isPrint = true;
        }

        return (
            <div>
                {isPrint &&
                    <div>
                        <h5>Enter the code for validation or scan QR Code</h5>
                        <InputGroup className="mb-3">
                            <InputGroup.Prepend>
                                <InputGroup.Text id="inputGroup-sizing-default">Key</InputGroup.Text>
                            </InputGroup.Prepend>
                            <FormControl
                                name="key"
                                value={key}
                                label="Key"
                                onChange={this.handleChange}
                                aria-label="Default"
                                aria-describedby="inputGroup-sizing-default"
                            />
                            <InputGroup.Append>
                                <Button
                                    onClick={this.handleClick}
                                    variant='primary'
                                >
                                    ORDER TAKEN
                                </Button>
                            </InputGroup.Append>
                        </InputGroup>
                        <ReadQRCode dataQRCode={this.handleReadQRCode} />
                    </div>
                }
            </div>
        )
    }
}

const mapStateToProps = state => state;

const mapDispatchToProps = {
    setOrder: OrderAction.setOrder,
};

export default connect(mapStateToProps, mapDispatchToProps)(OrderTaken);