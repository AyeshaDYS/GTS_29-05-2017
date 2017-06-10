<?php
/* Definition of the purch_order class to hold all the information for a purchase order and delivery
*/

class supp_payable_order
{

	var $trans_type; // order/grn/invoice (direct)
	var $line_items;
	var $curr_code;
	var $supp_ref;
	var $delivery_address;
	var $Comments;
	var $Location;
	var $supplier_id;
	var $supplier_name;
	var $orig_order_date;
	var $due_date;
	var $order_no; /*Only used for modification of existing orders otherwise only established when order committed */
	var $lines_on_order = 0;
	var $credit;
	var $tax_group_id;
	var $tax_group_array = null; // saves db queries
	var $tax_included; // type of prices
	var $terms;
	var $e_model;
	var $e_serial;
	var $cost_center;
	var $category_id;
	var $long_desc;
 	var $qty_invoiced;
 	var $Purch_Req;
	var $reference;
    var $Type2;
    var $Type3;
    var $Adjust_By;
    var $t_series;
    var $pr_type;
    var $material;
    var $invoice_no;

	var $address_origin;
	var $note;
	var $p_note;
	var $text;
	var $regards;
	var $company;
	var $dear_sir;
	var $deadline ;
	var $inco_terms ;
	var $port_of_loading ;
	var $port_of_destination ;
	var $port_of_destination_airport;
	var $shipment_mode;
	var $product ;
	var $Qty;
	var $gross_weight1 ;
	var $units_g ;
	var $net_weight1 ;
	var $units_n ;
	var $volumetric_weight1  ;
	var $units_v  ;

	function supp_payable_order()
	{
		/*Constructor function initialises a new purchase order object */
		$this->line_items = array();
		$this->lines_on_order = $this->order_no = $this->supplier_id = 0;
	} 

	function set_supplier_supp_payable($supplier_id, $supplier_name, $curr_code, $tax_group_id, $tax_included)
	{
		$this->supplier_id = $supplier_id;
		$this->supplier_name = $supplier_name;
		$this->curr_code = $curr_code;
		$this->tax_group_id = $tax_group_id;
		$this->tax_included = $tax_included;
		$this->tax_group_array = get_tax_group_items_as_array($tax_group_id);
	}

	function add_to_order_supp_payable($line_no,$qty,$pkg_type,$length,$width,$height,$volumetric_weight
,$units1, $gross_weight,$units2,$net_weight,$units3)	{

		if ($qty != 0 && isset($qty))
		{

			$this->line_items[$line_no] = new po_line_details_supp_payable($line_no, $qty,$pkg_type,$length,$width,$height,$volumetric_weight
				,$units1, $gross_weight,$units2,$net_weight,$units3);
			$this->lines_on_order++;
			return 1;
		}
		return 0;
	}

	function update_order_item_supp_payable($line_no,$qty,$pkg_type,$length,$width,$height,
										   $volumetric_weight,$units1, $gross_weight,$units2,
										   $net_weight,$units3)
	{
//		if ($description != "")
//			$this->line_items[$line_no]->item_description = $description;
//
//		if ($long_desc != "")
			$this->line_items[$line_no]->qty = $qty;
		$this->line_items[$line_no]->pkg_type = $pkg_type;
		$this->line_items[$line_no]->length = $length;
		$this->line_items[$line_no]->width = $width;
		$this->line_items[$line_no]->height = $height;
		$this->line_items[$line_no]->volumetric_weight = $volumetric_weight;
		$this->line_items[$line_no]->units1 = $units1;
		$this->line_items[$line_no]->gross_weight = $gross_weight;
		$this->line_items[$line_no]->units2 = $units2;
		$this->line_items[$line_no]->net_weight = $net_weight;
		$this->line_items[$line_no]->units3 = $units3;
	}

	function remove_from_order_supp_payable($line_no)
	{
		array_splice($this->line_items, $line_no, 1);
	}
	
	function order_has_items_supp_payable()
	{
		return count($this->line_items) != 0;
	}
	
	function clear_items_supp_payable()
	{
    	unset($this->line_items);
		$this->line_items = array();
		
		$this->lines_on_order = 0;  
		$this->order_no = 0;
	}

	
	function any_already_received_supp_payable()
	{
		/* Checks if there have been deliveries or invoiced entered against any of the line items */
		if (count($this->line_items) > 0)
		{
		   	foreach ($this->line_items as $ordered_items) 
		   	{
				if ($ordered_items->qty_received != 0 || $ordered_items->qty_inv != 0)
				{
					return 1;
				}
		   	}
		}
		return 0;
	}

	function some_already_received_supp_payable($line_no)
	{
		/* Checks if there have been deliveries or amounts invoiced against a specific line item */
		if (count($this->line_items) > 0)
		{
		   	if ($this->line_items[$line_no]->qty_received != 0 || 
		   		$this->line_items[$line_no]->qty_inv != 0)
		 	{
				return 1;
		   	}
		}
		return 0;
	}
	
	//
	//	Returns taxes for PO/GRN.
	//	$receival=true in purchase receive context.
	//
	function get_taxes_supp_payable($shipping_cost=null, $receival=false)
	{
		$items = array();
		$prices = array();
		if($shipping_cost==null)
			$shipping_cost = 0;//$this->freight_cost;

		foreach ($this->line_items as $ln_itm) {
			$items[] = $ln_itm->stock_id;
			$prices[] = round($ln_itm->price * ($receival ? $ln_itm->receive_qty : $ln_itm->quantity),  user_price_dec());
		}
		$taxes = get_tax_for_items($items, $prices, $shipping_cost,
		  $this->tax_group_id, $this->tax_included,  $this->tax_group_array);

    	// Adjustment for swiss franken, we always have 5 rappen = 1/20 franken
	    if ($this->curr_code == 'CHF') {
			$val = $taxes['1']['Value'];
			$val1 = (floatval((intval(round(($val*20),0)))/20));
			$taxes['1']['Value'] = $val1;
		} 
		return $taxes;
	}

	/*
		Returns order value including all taxes
	*/
	function get_trans_total_supp_payable() {
		
		$total = 0;
		$dec = user_price_dec();

		foreach ($this->line_items as $ln_itm) {
			$items[] = $ln_itm->stock_id;
			$value = round($ln_itm->quantity * $ln_itm->price, $dec);
			$prices[] =$value;
			$total += $value;
		}

		if (!$this->tax_included ) {
			$taxes = get_tax_for_items($items, $prices, 0, $this->tax_group_id,
			$this->tax_included,  $this->tax_group_array);

			foreach($taxes as $tax)
				$total += round($tax['Value'], $dec);
		}
		return $total;
	}

} /* end of class defintion */

class po_line_details_supp_payable
{

	var $line_no;
	var $qty;
    var $pkg_type;
	var $length;
	var $width;
	var $height;
	var $volumetric_weight;
	var $units1;
	var $gross_weight;
	var $units2;
	var $net_weight;

	var $units3;
	

	function po_line_details_supp_payable($line_no,$qty,$pkg_type,$length,$width,$height,$volumetric_weight
		,$units1, $gross_weight,$units2,$net_weight,$units3)
	{
		/* Constructor function to add a new LineDetail object with passed params */
		
		$this->line_no = $line_no;
		$this->qty=$qty;
		$this->pkg_type=$pkg_type;
		$this->length=$length;
		$this->width=$width;
		$this->height=$height;
		$this->volumetric_weight=$volumetric_weight;
		$this->units1=$units1;
		$this->gross_weight=$gross_weight;
		$this->units2=$units2;
		$this->net_weight=$net_weight;
		$this->units3=$units3;

//		$this->e_model = $e_model;
//		$this->e_serial = $e_serial;
	}
	
	function taxfree_charge_price($po)
	{
		return get_tax_free_price_for_item($this->stock_id, $this->price, 
		  $po->tax_group_id, $po->tax_included, $po->tax_group_array);
	}
}

?>
