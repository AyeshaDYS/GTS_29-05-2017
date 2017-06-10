<?php
/* Definition of the Supplier Transactions class to hold all the information for an accounts payable invoice or credit note
*/

include_once($path_to_root . "/taxes/tax_calc.inc");

class supp_trans_credit
{

    var $grn_items; /*array of objects of class grn_item using the id as the pointer */
    var $gl_codes; /*array of objects of class gl_codes using a counter as the pointer */
    var $supplier_id;
    var $supplier_name;
    var $terms;
    var $tax_description;
    var $tax_group_id;
    var $tax_included;
    var $trans_type;	// invoice or credit
    var $Comments;
    var $tran_date;
    var $due_date;
    var $supp_ref;
    var $material;
    var $material1;
    var $s_consol;
    var $po_type_ref;
    var $status_type;
    var $repairing_ref;
    var $repairing_ref_cs;
    var $pr_no;
    var $readyDate;
    var $eta;
    var $freight_fowarder;
    var $weight_dimension;
    var $cn_ref;
    var $e_model;
    var $e_serial;
    var $payment_terms;
    var $cost_center;
    var $category_id;
    var $item_title;
    var $pr_ref;
    var $pr_type_ref;
    var $import_num;
    var $purchase_id;
    var $po_id;
    var $Consignment_no;
    var $cn_id;
    var $po_ref;
    var $pr_voucher_no;
    var $delivery_date;
    var $claim_status;
    var $stock_status;
    var $delivery_terms;
    var $requiredfor;
    var $material_centre;
    var $shipment;
    var $supp_reference;
    var $gst_no1;
    var $reference;
    var $ov_amount;
    var $ov_discount;
    var $ov_gst;
    var $gl_codes_counter=0;
    var $credit = 0;
    var $transaction_series;
    var $transaction_type;
    var $voucher_date;
    var $foward_date;
    var $po_date;
    var $pr_type;
    var $type2;
    var $grn_book_no;
    var $grn_no;
    var $supp_invoice_num;
    var $supp_dc_num;
    var $supp_invoice_date;
    var $supp_dc_date;
    var $shipment_date;
    var $serial_no;
    var $select_pr;
    var $pr_date;
    var $claim_for_file;
    var $acknowledgement_no;
    var $grn_transaction_id;
    var $repairing_reference;
    var $followup_advice_outcome;
    var $fault_rectification_advice;
    var $supp_fault_rectification_advice;
    var $supp_fault_rectification;
    var $claim_foward;
    var $fault_description;
    var $supp_invoice_no;
    var $dispatch_to_supplier;
    var $supp_repairing_ref;
    var $rep_transaction_id;
    var $rep_grn_transaction_id;
    var $dispatch_status;
    var $Date;
    var $inspection;
    var $order_to_supplier;
    var $gts_dispatch_via;
    var $dispatch_date;
    var $gts_dispatch_tracking;
    var $order_date;
    var $supp_dispatch_via;
    var $long_desc;
    var $shipment_console;
    var $acknowledge_status;
    var $order_acknowledgement;
    var $acknowledgment_date;
    var $gst_po_no;
    var $freight_forwarder;
    var $etd;
    var $arrival_date;
    var $gts_dispatch_date;
    var $supp_dispatch_date;
    var $po_no1;
    var $supp_repairing_no;
    var $get_grn_id;
    var $purchase_date;
    var $ready_date;
    var $ord_no;
    var $supp_repairing_grn_no;
    var $pending_status;

    function supp_trans_credit($trans_type, $type2,$grn_id)
    {
        $this->trans_type = $trans_type;
        $this->local = $type2;
        $this->grn_id = $grn_id;

        /*Constructor function initialises a new Supplier Transaction object */
        $this->grn_items = array();
        $this->gl_codes = array();
    }

    function add_grn_to_trans_credit($grn_item_id, $po_detail_item, $item_code, $item_description,
                              $qty_recd, $prev_quantity_inv, $this_quantity_inv, $order_price, $chg_price,
                              $std_cost_unit, $gl_code, $cost_centre, $long_description, $part_no, $Transaction_Series,
                              $required_for, $supp_invoice_no, $material_centre)
    {


        $this->grn_items[$grn_item_id] = new grn_item_credit($grn_item_id, $po_detail_item,
            $item_code, $item_description, $qty_recd, $prev_quantity_inv, $this_quantity_inv,
            $order_price, $chg_price, $std_cost_unit, $gl_code, $this->tax_included, $cost_centre,
            $long_description, $part_no, $Transaction_Series, $required_for, $supp_invoice_no, $material_centre);
        return 1;
    }

    function add_gl_codes_to_trans_credit($gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_)
    {
        $this->gl_codes[$this->gl_codes_counter] = new gl_codes($this->gl_codes_counter,
            $gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_);
        $this->gl_codes_counter++;
        return 1;
    }

    function remove_grn_from_trans_credit($grn_item_id)
    {
        unset($this->grn_items[$grn_item_id]);
    }
    function remove_gl_codes_from_trans_credit($gl_code_counter)
    {
        unset($this->gl_codes[$gl_code_counter]);
    }

    function is_valid_trans_to_post()
    {
        return (count($this->grn_items) > 0 || count($this->gl_codes) > 0 ||
            ($this->ov_amount != 0) || ($this->ov_discount > 0));
    }

    function clear_items_credit()
    {
        unset($this->grn_items);
        unset($this->gl_codes);
        $this->ov_amount = $this->ov_discount = $this->supplier_id = 0;

        $this->grn_items = array();
        $this->gl_codes = array();
    }

    function get_taxes($tax_group_id=null, $shipping_cost=0, $gl_codes=true)
    {
        $items = array();
        $prices = array();

        if ($tax_group_id == null)
            $tax_group_id = $this->tax_group_id;

        // preload the taxgroup !
        $tax_group = get_tax_group_items_as_array($tax_group_id);

        foreach ($this->grn_items as $ln_itm)
        {
            $items[] = $ln_itm->item_code;
//        	$prices[] =round( ($ln_itm->this_quantity_inv * $ln_itm->taxfree_charge_price($tax_group_id, $tax_group)),
            $prices[] =round( ($ln_itm->this_quantity_inv * $ln_itm->chg_price),
                user_price_dec());
        }

        if ($tax_group_id == null)
            $tax_group_id = $this->tax_group_id;
        $taxes = get_tax_for_items($items, $prices, $shipping_cost, $tax_group_id,
            $this->tax_included);

///////////////// Joe Hunt 2009.08.18

        if ($gl_codes)
        {
            foreach ($this->gl_codes as $gl_code)
            {
                $index = is_tax_account($gl_code->gl_code);
                if ($index !== false)
                {
                    $taxes[$index]['Value'] += $gl_code->amount;
                }
            }
        }
        return $taxes;
    }
    //
    //	Returns total invoice amount without taxes.
    //
    function get_total_taxfree($tax_group_id=null)
    {
        $total = 0;

        // preload the taxgroup !
        if ($tax_group_id != null)
            $tax_group = get_tax_group_items_as_array($tax_group_id);
        else
            $tax_group = null;

        foreach ($this->grn_items as $ln_itm)
            $total += round(($ln_itm->this_quantity_inv * $ln_itm->taxfree_charge_price($tax_group_id, $tax_group)),
                user_price_dec());

        foreach ($this->gl_codes as $gl_line)
        {	//////// 2009-08-18 Joe Hunt
            if (!is_tax_account($gl_line->gl_code))
                $total += $gl_line->amount;
        }
        return $total;
    }

    function get_items_total()
    {
        $total = 0;

        foreach ($this->grn_items as $ln_itm)
            $total += round($ln_itm->this_quantity_inv * $ln_itm->chg_price, user_price_dec());

        foreach ($this->gl_codes as $gl_line)
        {   //////// 2010-10-10 Joe Hunt
            if (!is_tax_account($gl_line->gl_code) || $this->tax_included)
                $total += $gl_line->amount;
        }
        return $total;
    }
} /* end of class defintion */

class grn_item_credit
{

    /* Contains relavent information from the purch_order_details as well to provide in cached form,
    all the info to do the necessary entries without looking up ie additional queries of the database again */

    var $id;
    var $po_detail_item;
    var $item_code;
    var $item_description;
    var $qty_recd;
    var $prev_quantity_inv;
    var $this_quantity_inv;
    var $order_price;
    var $chg_price;
    var $std_cost_unit;
    var $gl_code;
    var $tax_included;
    var $cost_centre;
    var $long_description;
    var $material_centre;
    var $requiredfor;
    var $transaction_series;
    var $stock_status;
    var $warranty_period;
    var $total_warranty_period;


    function grn_item_credit($id, $po_detail_item, $item_code, $item_description, $qty_recd,
                      $prev_quantity_inv, $this_quantity_inv, $order_price, $chg_price,
                      $std_cost_unit, $gl_code, $tax_included, $cost_centre, $long_description, $part_no,
                      $Transaction_Series, $required_for, $supp_invoice_no, $material_centre,$stock_status,$warranty_period,$total_warranty_period)
    {


        $this->id = $id;
        $this->po_detail_item = $po_detail_item;
        $this->item_code = $item_code;
        $this->item_description = $item_description;
        $this->qty_recd = $qty_recd;
        $this->prev_quantity_inv = $prev_quantity_inv;
        $this->this_quantity_inv = $this_quantity_inv;
        $this->order_price =$order_price;
        $this->chg_price = $chg_price;
        $this->std_cost_unit = $std_cost_unit;
        $this->gl_code = $gl_code;
        $this->tax_included = $tax_included;
        $this->cost_centre = $cost_centre;
        $this->long_description = $long_description;
        $this->part_no = $part_no;
        $this->transaction_series = $Transaction_Series;
        $this->required_for = $required_for;
        $this->supp_invoice_no = $supp_invoice_no;
        $this->material_centre = $material_centre;
        $this->stock_status=$stock_status;
        $this->warranty_period=$warranty_period;
        $this->total_warranty_period=$total_warranty_period;
    }

    function full_charge_price($tax_group_id, $tax_group=null)
    {
        return get_full_price_for_item($this->item_code,
            $this->chg_price, $tax_group_id, $this->tax_included, $tax_group);
    }

    function taxfree_charge_price($tax_group_id, $tax_group=null)
    {
//		if ($tax_group_id==null)
//			return $this->chg_price;
        return get_tax_free_price_for_item($this->item_code, $this->chg_price,
            $tax_group_id, $this->tax_included, $tax_group);
    }
}


class gl_codes_credit
{

    var $Counter;
    var $gl_code;
    var $gl_act_name;
    var $gl_dim;
    var $gl_dim2;
    var $amount;
    var $memo_;

    function gl_codes_credit ($Counter, $gl_code, $gl_act_name, $gl_dim, $gl_dim2, $amount, $memo_)
    {

        /* Constructor function to add a new gl_codes object with passed params */
        $this->Counter = $Counter;
        $this->gl_code = $gl_code;
        $this->gl_act_name = $gl_act_name;
        $this->gl_dim = $gl_dim;
        $this->gl_dim2 = $gl_dim2;
        $this->amount = $amount;
        $this->memo_= $memo_;
    }
}

?>
