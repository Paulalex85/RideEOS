import React, { Component } from 'react';
import { UserProfile, Logout } from 'components';

import InputLabel from '@material-ui/core/InputLabel';
import MenuItem from '@material-ui/core/MenuItem';
import FormControl from '@material-ui/core/FormControl';
import Select from '@material-ui/core/Select';
import FormAPI from './FormAPI';
import KeyGenerator from './KeyGenerator';

import { connect } from 'react-redux';
// Services and redux action
import { UserAction } from 'actions';
import OrderDashboard from '../OrderDashboard/OrderDashboard';
import MarketDashboard from '../MarketDashboard/MarketDashboard';

class FormManager extends Component {
    constructor(props) {
        super(props)

        // Bind functions
        this.handleChange = this.handleChange.bind(this);
        this.handleEosChange = this.handleEosChange.bind(this);
    }

    state = {
        formToShow: 'adduser',
        privateKey: '',
        account: '',
        publicKey: '',
    };

    handleChange(event) {
        this.setState({ formToShow: event.target.value });
    }

    handleEosChange(name, value) {
        this.setState({
            [name]: value
        });
    }

    render() {
        return (
            <div>
                <Logout />
                <UserProfile />
                <MarketDashboard />
                <OrderDashboard />
                <FormControl>
                    <InputLabel>Send to EOS</InputLabel>
                    <Select
                        value={this.state.formToShow}
                        onChange={this.handleChange}
                    >
                        <MenuItem value={'adduser'}>Add</MenuItem>
                        <MenuItem value={'updateuser'}>Update</MenuItem>
                        <MenuItem value={'deposit'}>Deposit</MenuItem>
                        <MenuItem value={'withdraw'}>Withdraw</MenuItem>
                        <MenuItem value={'needdeliver'}>Need Deliver</MenuItem>
                        <MenuItem value={'deliverfound'}>Deliver Found</MenuItem>
                        <MenuItem value={'initialize'}>Create Order</MenuItem>
                        <MenuItem value={'validatebuy'}>Buyer validate</MenuItem>
                        <MenuItem value={'validatesell'}>Seller validate</MenuItem>
                        <MenuItem value={'validatedeli'}>Deliver validate</MenuItem>
                        <MenuItem value={'orderready'}>Order Ready</MenuItem>
                        <MenuItem value={'ordertaken'}>Order Taken</MenuItem>
                        <MenuItem value={'orderdelive'}>Order Delive</MenuItem>
                        <MenuItem value={'initcancel'}>Initialization Cancel</MenuItem>
                        <MenuItem value={'delaycancel'}>Delay Cancel</MenuItem>
                        <MenuItem value={'newassign'}>New Assign</MenuItem>
                        <MenuItem value={'endassign'}>End Assign</MenuItem>
                        <MenuItem value={'addoffer'}>Add Offer</MenuItem>
                        <MenuItem value={'endoffer'}>End Offer</MenuItem>
                        <MenuItem value={'canceloffer'}>Cancel Offer</MenuItem>
                        <MenuItem value={'addapply'}>Add Apply</MenuItem>
                        <MenuItem value={'cancelapply'}>Cancel Apply</MenuItem>
                    </Select>
                </FormControl>
                <FormControl>
                    <FormAPI
                        globalInfo={this.state}
                        onChange={this.handleEosChange}
                    />
                    <KeyGenerator />
                </FormControl>
            </div>
        );
    }
}

// Map all state to component props (for redux to connect)
const mapStateToProps = state => state;

// Map the following action to props
const mapDispatchToProps = {
    setUser: UserAction.setUser,
};

// Export a redux connected component
export default connect(mapStateToProps, mapDispatchToProps)(FormManager);