import { pgTable, serial, varchar, boolean, timestamp, integer, index } from "drizzle-orm/pg-core"
import { sql } from "drizzle-orm"
import { createSchemaFactory } from "drizzle-zod"
import { z } from "zod"

// 座位表
export const seats = pgTable(
  "seats",
  {
    id: serial().notNull().primaryKey(),
    name: varchar("name", { length: 100 }).notNull(),
    location: varchar("location", { length: 255 }),
    deviceId: integer("device_id").unique(),
    isOccupied: boolean("is_occupied").default(false).notNull(),
    createdAt: timestamp("created_at", { withTimezone: true, mode: 'string' }).defaultNow().notNull(),
    updatedAt: timestamp("updated_at", { withTimezone: true, mode: 'string' }).defaultNow(),
  },
  (table) => [
    index("seats_device_id_idx").on(table.deviceId),
    index("seats_name_idx").on(table.name),
  ]
);

// 设备表
export const devices = pgTable(
  "devices",
  {
    id: serial().notNull().primaryKey(),
    deviceKey: varchar("device_key", { length: 64 }).notNull().unique(),
    deviceName: varchar("device_name", { length: 100 }).notNull(),
    seatId: integer("seat_id").unique(),
    status: varchar("status", { length: 20 }).default("offline").notNull(),
    lastHeartbeat: timestamp("last_heartbeat", { withTimezone: true, mode: 'string' }),
    createdAt: timestamp("created_at", { withTimezone: true, mode: 'string' }).defaultNow().notNull(),
    updatedAt: timestamp("updated_at", { withTimezone: true, mode: 'string' }).defaultNow(),
  },
  (table) => [
    index("devices_device_key_idx").on(table.deviceKey),
    index("devices_seat_id_idx").on(table.seatId),
  ]
);

// 座位状态记录表
export const seatStatusLogs = pgTable(
  "seat_status_logs",
  {
    id: serial().notNull().primaryKey(),
    seatId: integer("seat_id").notNull(),
    isOccupied: boolean("is_occupied").notNull(),
    recordedAt: timestamp("recorded_at", { withTimezone: true, mode: 'string' }).defaultNow().notNull(),
  },
  (table) => [
    index("seat_status_logs_seat_id_idx").on(table.seatId),
    index("seat_status_logs_recorded_at_idx").on(table.recordedAt),
  ]
);

// 系统健康检查表
export const healthCheck = pgTable("health_check", {
	id: serial().notNull(),
	updatedAt: timestamp("updated_at", { withTimezone: true, mode: 'string' }).defaultNow(),
});

// Schema factory for Zod validation
const { createInsertSchema: createCoercedInsertSchema } = createSchemaFactory({
  coerce: { date: true },
});

// Zod schemas for seats
export const insertSeatSchema = createCoercedInsertSchema(seats).pick({
  name: true,
  location: true,
  deviceId: true,
});

export const updateSeatSchema = createCoercedInsertSchema(seats)
  .pick({
    name: true,
    location: true,
    deviceId: true,
    isOccupied: true,
  })
  .partial();

// Zod schemas for devices
export const insertDeviceSchema = createCoercedInsertSchema(devices).pick({
  deviceKey: true,
  deviceName: true,
  seatId: true,
});

export const updateDeviceSchema = createCoercedInsertSchema(devices)
  .pick({
    deviceName: true,
    seatId: true,
    status: true,
    lastHeartbeat: true,
  })
  .partial();

// Zod schemas for seat status logs
export const insertSeatStatusLogSchema = createCoercedInsertSchema(seatStatusLogs).pick({
  seatId: true,
  isOccupied: true,
});

// TypeScript types
export type Seat = typeof seats.$inferSelect;
export type InsertSeat = z.infer<typeof insertSeatSchema>;
export type UpdateSeat = z.infer<typeof updateSeatSchema>;

export type Device = typeof devices.$inferSelect;
export type InsertDevice = z.infer<typeof insertDeviceSchema>;
export type UpdateDevice = z.infer<typeof updateDeviceSchema>;

export type SeatStatusLog = typeof seatStatusLogs.$inferSelect;
export type InsertSeatStatusLog = z.infer<typeof insertSeatStatusLogSchema>;
