package de.fraunhofer.itwm.hexabus;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;
import java.sql.Timestamp;
import java.util.Calendar;

public class HexabusWritePacket extends HexabusPacket {
	
	private Hexabus.DataType dataType;
	private byte[] payload;
	protected byte eid;

	public HexabusWritePacket(int eid, Hexabus.DataType dataType, byte[] payload) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = dataType;
		if(payload.length>dataType.getSize()) {
			throw new Hexabus.HexabusException("Payload too large");
		}
		this.payload = payload;
	}
		
	public HexabusWritePacket(int eid, boolean value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.BOOL;
		this.payload = new byte[] {(byte) (value?0x01:0x00)};
	}

	public HexabusWritePacket(int eid, float value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.FLOAT;
		this.payload = new byte[4];
		ByteBuffer buffer = ByteBuffer.wrap(payload);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		buffer.putFloat(value);
	}

	public HexabusWritePacket(int eid, short value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.UINT8;
		// uint8 value as short
		if(value>255) {
			throw new Hexabus.HexabusException("Value too large. UINT8 expected.");
		}
		this.payload = new byte[] {(byte) (value & 0xFF)};
	}

	public HexabusWritePacket(int eid, long value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.UINT32;
		this.payload = new byte[] {(byte) ((value & 0xFF000000L) >> 24)
			,(byte) ((value & 0x00FF0000L) >> 16)
			,(byte) ((value & 0x0000FF00L) >> 8)
			,(byte) (value & 0x000000FFL)};
	}

	public HexabusWritePacket(int eid, Timestamp value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.TIMESTAMP;
		long timestamp = value.getTime();
		this.payload = new byte[] {(byte) ((timestamp & 0xFF000000L) >> 24)
			,(byte) ((timestamp & 0x00FF0000L) >> 16)
			,(byte) ((timestamp & 0x0000FF00L) >> 8)
			,(byte) (timestamp & 0x000000FFL)};
	}

	public HexabusWritePacket(int eid, Calendar value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.DATETIME;
		this.payload = new byte[dataType.getSize()];
		ByteBuffer buffer = ByteBuffer.wrap(payload);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		buffer.put((byte) (value.get(Calendar.HOUR_OF_DAY) & 0xFF));
		buffer.put((byte) (value.get(Calendar.MINUTE) & 0xFF));
		buffer.put((byte) (value.get(Calendar.SECOND) & 0xFF));
		buffer.put((byte) (value.get(Calendar.DAY_OF_MONTH) & 0xFF));
		buffer.put((byte) ((value.get(Calendar.MONTH)+1) & 0xFF));
		buffer.put((byte) ((value.get(Calendar.YEAR) & 0xFF00) >> 8));
		buffer.put((byte) (value.get(Calendar.YEAR) & 0x00FF));
		buffer.put((byte) ((value.get(Calendar.DAY_OF_WEEK)-1) & 0xFF));
	}

	public HexabusWritePacket(int eid, String value) throws Hexabus.HexabusException {
		this.packetType = Hexabus.PacketType.WRITE;
		if(eid>255) {
			throw new Hexabus.HexabusException("EID too large");
		}
		if(value.length()>127) {
			throw new Hexabus.HexabusException("String too long");
		// check charset
		} else if(!Charset.forName("US-ASCII").newEncoder().canEncode(value)) {
			throw new Hexabus.HexabusException("Non-ascii string");
		}
	
		this.eid = (byte) eid;
		this.dataType = Hexabus.DataType.STRING;
		this.payload = new byte[dataType.getSize()];
		ByteBuffer buffer = ByteBuffer.wrap(payload);
		buffer.order(ByteOrder.BIG_ENDIAN);		
		try{
			buffer.put(value.getBytes("US-ASCII"));
		}catch(Exception e){throw new Hexabus.HexabusException("Non-existing charset. Please report to author.");}//TODO

		for(int i=0; i<=(127-value.length());i++) { // Fill payload field with zeros
			buffer.put((byte) 0x00);
		}
	}

	public Hexabus.DataType getDataType() {
		return dataType;
	}

	public int getEid() {
		return eid;
	}

	public boolean getBool() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.BOOL) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseBool(payload);
	}

	public short getUint8() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.UINT8) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseUint8(payload);
	}

	public String getString() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.STRING) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseString(payload);
	}

	public float getFloat() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.FLOAT) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseFloat(payload);
	}

	public long getUint32() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.UINT32) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseUint32(payload);
	}

	public Timestamp getTimestamp() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.TIMESTAMP) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseTimestamp(payload);
	}

	public Calendar getDatetime() throws Hexabus.HexabusException {
		if(dataType != Hexabus.DataType.DATETIME) {
			throw new Hexabus.HexabusException("Wrong data type. "+dataType+" expected.");
		}
		return Hexabus.parseDatetime(payload);
	}

	protected byte[] buildPacket() throws Hexabus.HexabusException {
		byte[] packetData = new byte[packetType.getBaseLength()+dataType.getSize()-2];
		ByteBuffer buffer = super.buildPacket(packetData);

		buffer.put(eid);
		buffer.put(dataType.convert());
		buffer.put(payload);

		return packetData;
	}

}
